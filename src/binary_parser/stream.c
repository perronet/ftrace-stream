#include "stream.h"

// TODO rewrite this without these horrible static variables and gotos
struct tracecmd_input *init_stream(int read_fd, int cpu, int cpu_cnt) {
	struct tracecmd_output *trace_output;
	static struct tracecmd_input *trace_input;
	static FILE *fp = NULL;
	int fd1;
	int fd2;
	long flags;

	if (fp && trace_input)
		goto make_pipe;

	// Create temporary file
	fp = tmpfile();
	if (!fp)
		return NULL;
	fd1 = fileno(fp);
	fd2 = dup(fd1);

	// Write tracecmd binary header in the file to pretend that we are reading from a valid trace.dat file
	trace_output = tracecmd_create_init_fd(fd2);
	if (!trace_output) {
		fclose(fp);
		return NULL;
	}
	tracecmd_output_free(trace_output);

	lseek(fd2, 0, SEEK_SET);

	// Get handle for event stream. This function will check that the fd corresponds to a valid trace.dat file
	trace_input = tracecmd_alloc_fd(fd2, 0);
	if (!trace_input) {
		close(fd2);
		goto fail;
	}

	// Consume binary header
	if (tracecmd_read_headers(trace_input, TRACECMD_FILE_PRINTK) < 0)
		goto fail_free_input;

make_pipe:
	/* Do not block on this pipe */
	flags = fcntl(read_fd, F_GETFL);
	fcntl(read_fd, F_SETFL, flags | O_NONBLOCK);

	if (tracecmd_make_pipe(trace_input, cpu, read_fd, cpu_cnt) < 0)
		goto fail_free_input;

	return trace_input;

fail_free_input:
	tracecmd_close(trace_input);
fail:
	fclose(fp);

	return NULL;
}

/* Read a single event, parse it into "rbf_event" in our format */
/* Returns 1 if an event was read, 0 otherwise (e.g. there were no events to read) */
int read_stream(struct recorder_data *recorders, int cpu_cnt, struct rbftrace_event *rbf_event) {
	struct tep_record *record;
	struct recorder_data *rec;
	struct recorder_data *last_rec;
	struct tep_handle *event_parser;
	struct tep_event *event;
	struct timeval tv = { 1 , 0 };
	fd_set rfds;
	int top_rfd = 0;
	int nr_fd;
	int ret;
	int i;

	last_rec = NULL;

	/* Reads a record for each recorder thread */
 again:
	for (i = 0; i < cpu_cnt; i++) {
		rec = &recorders[i];

		if (!rec->record)
			rec->record = tracecmd_read_data(rec->stream, rec->cpu);
		record = rec->record;
		/* Pipe has closed */
		if (!record && errno == EINVAL)
			rec->closed = 1;

		/* Picks the smallest timestamp */
		if (record && (!last_rec || record->ts < last_rec->record->ts))
			last_rec = rec;
	}
	/* Find the event */
	if (last_rec) {
		record = last_rec->record;
		last_rec->record = NULL;

		event_parser = tracecmd_get_tep(last_rec->stream);
		/* Most recent event. The most recent timestamp is stored in the record. */
		event = tep_find_event_by_record(event_parser, record);

		if (rbf_event != NULL) {
			if (parse_event_rbftrace(event, rbf_event, record) < 0) {
				printf("Parser error: field not found\n");
			}
		}

		tracecmd_free_record(record);

		return 1;
	}

	nr_fd = 0;
	FD_ZERO(&rfds);

	for (i = 0; i < cpu_cnt; i++) {
		/* Do not process closed pipes */
		if (recorders[i].closed)
			continue;
		nr_fd++;
		if (recorders[i].event_pipe[0] > top_rfd)
			top_rfd = recorders[i].event_pipe[0];

		FD_SET(recorders[i].event_pipe[0], &rfds);
	}

	if (!nr_fd)
		return 0;

	ret = select(top_rfd + 1, &rfds, NULL, NULL, &tv);

	if (ret > 0)
		goto again;

	return ret;
}

/* prev_state mapping
0 => R
1 => S
2 => D
...
16 => X
32 => Z
256 => R+ (The flag at 256 seems to trigger the "+")

{ 0x0000, "R" }, 
{ 0x0001, "S" }, 
{ 0x0002, "D" }, 
{ 0x0004, "T" }, 
{ 0x0008, "t" }, 
{ 0x0010, "X" }, 
{ 0x0020, "Z" },
{ 0x0040, "P" }, 
{ 0x0080, "I" }
*/

int parse_event_rbftrace(struct tep_event *source, struct rbftrace_event *target, struct tep_record *record) {
	unsigned long long val;
	unsigned short id;
	char *pidstr;
	char *priostr;

	target->ts = record->ts;
	if (tep_get_common_field_val(NULL, source, "common_type", record, &val, 0) < 0) {
		printf("Field common_type not found\n");
		return -1;
	}
	id = val;

	pidstr = (id == SWITCH_ID) ? "prev_pid" : "pid";
	priostr = (id == SWITCH_ID) ? "prev_prio" : "prio";

	if (tep_get_any_field_val(NULL, source, pidstr, record, &val, 0) < 0) {
		printf("Field pid not found\n");
		return -1;
	}
	target->pid = val;
	if (tep_get_field_val(NULL, source, priostr, record, &val, 0) < 0) {
		printf("Field prio not found\n");
		return -1;
	}
	target->prio = val;

	switch (id) {
		case WAKEUP_NEW_ID:
		case WAKEUP_ID:
			target->typ = WAKEUP;

			if (tep_get_field_val(NULL, source, "success", record, &val, 0) < 0) {
				printf("Field success not found\n");
				return -1;
			}
			target->success = val;
			if (tep_get_field_val(NULL, source, "target_cpu", record, &val, 0) < 0) {
				printf("Field target_cpu not found\n");
				return -1;
			}
			target->target_cpu = val;

			break;

		case SWITCH_ID:
			target->typ = SWITCH;

			if (tep_get_field_val(NULL, source, "prev_state", record, &val, 0) < 0) {
				printf("Field prev_state not found\n");
				return -1;
			}
			target->prev_state = val;
			if (tep_get_field_val(NULL, source, "next_pid", record, &val, 0) < 0) {
				printf("Field next_pid not found\n");
				return -1;
			}
			target->next_pid = val;
			if (tep_get_field_val(NULL, source, "next_prio", record, &val, 0) < 0) {
				printf("Field next_prio not found\n");
				return -1;
			}
			target->next_prio = val;

			break;

		case EXIT_ID:
			target->typ = EXIT;
			break;
	}

	return 0;
}

/* Debug */
void print_event(struct rbftrace_event *event) {
	switch (event->typ) {
		case WAKEUP:
			printf("%lld Wakeup typ %d pid %d prio %d success %d cpu %d\n", event->ts, event->typ, event->pid, event->prio, event->success, event->target_cpu);

			break;
		case SWITCH:
			printf("%lld Switch typ %d pid %d prio %d prev_state %ld next_pid %d next_prio %d \n", event->ts, event->typ, event->pid, event->prio, event->prev_state, event->next_pid, event->next_prio);

			break;
		default:
			printf("%lld Event typ %d pid %d prio %d\n", event->ts, event->typ, event->pid, event->prio);

			break;
	}
}
