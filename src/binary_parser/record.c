#include "stream.h"

// Used by each child recorder after forking. Each child has its own "recorder" instance
static struct tracecmd_recorder *recorder;
static int sleep_time = 1000;
// If true, the thread must stop (could be the main thread or a child recorder)
static int finished = 0;

void finish() {
	if (recorder)
		tracecmd_stop_recording(recorder); // Only executed by child recorders
	finished = 1;
}

void stop_threads(struct recorder_data *recorders, int cpu_cnt) {
	int ret;
	int i;
	struct rbftrace_event *event = malloc(sizeof(*event));

	/* Tell all threads to finish up */
	for (i = 0; i < cpu_cnt; i++) {
		if (recorders[i].pid > 0) {
			kill(recorders[i].pid, SIGUSR1);
		}
	}

	/* Flush out the pipes */
	do {
		ret = read_stream(recorders, cpu_cnt, event);
		printf("Flushed: ");
		print_event(event);
	} while (ret > 0);

	free(event);
}

void wait_threads(struct recorder_data *recorders, int cpu_cnt) {
	int i;

	for (i = 0; i < cpu_cnt; i++) {
		if (recorders[i].pid > 0) {
			waitpid(recorders[i].pid, NULL, 0);
			recorders[i].pid = -1;
			printf("Waited recorder #%d\n", i);
		}
	}
}

/* Returns recorder pid */
// TODO shall we set real-time priority? In that case, we might need to use add_filter_pid
int create_recorder(int cpu, int *event_pipe, char *tracefs_path) {
	pid_t pid;

	pid = fork();
	// Father
	if (pid != 0)
		return pid;

	// Child
	signal(SIGINT, SIG_IGN); // Ignore sigint
	signal(SIGUSR1, finish); // Stop on sigusr

	close(event_pipe[0]);
	recorder = tracecmd_create_buffer_recorder_fd(event_pipe[1], cpu, TRACECMD_RECORD_BLOCK_SPLICE, tracefs_path);
	tracefs_put_tracing_file(tracefs_path);
	if (!recorder) {
		printf("Can't create recorder\n");
		exit(-1);
	}

	while (!finished) {
		if (tracecmd_start_recording(recorder, sleep_time) < 0)
			break;
	}
	tracecmd_free_recorder(recorder);
	recorder = NULL;

	exit(0);
}

/* Create recorders: one for each cpu */
struct recorder_data *create_recorders(struct tracefs_instance *tracefs, int cpu_cnt) {
	struct recorder_data *recorders = calloc(cpu_cnt, sizeof(*recorders));
	char *tracefs_path = tracefs_instance_get_dir(tracefs);
	int *event_pipe = NULL;
	int ret;

	for (int i = 0; i < cpu_cnt; i++) {

		/* Setup recorder */
		recorders[i].cpu = i;
		recorders[i].record = NULL;
		event_pipe = recorders[i].event_pipe;
		ret = pipe(event_pipe);
		if (ret < 0) {
			printf("Pipe error\n");
			free(recorders);
			return NULL;
		}
		recorders[i].stream = init_stream(event_pipe[0], i, cpu_cnt);
		if (!recorders[i].stream) {
			printf("Stream error\n");
			free(recorders);
			return NULL;
		}
		fflush(stdout);

		/* Start recorder thread */
		ret = create_recorder(i, event_pipe, tracefs_path);
		recorders[i].pid = ret;
		if (ret < 0) {
			printf("Fork error\n");
			free(recorders);
			return NULL;
		}
		if (event_pipe)
			close(event_pipe[1]);
	}

	return recorders;
}
