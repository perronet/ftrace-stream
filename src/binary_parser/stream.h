#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

// trace-cmd libraries
#include <event-parse.h>
#include <tracefs.h>
#include <trace-cmd.h>

#define WAKEUP_ID 316
#define WAKEUP_NEW_ID 315
#define SWITCH_ID 314
#define EXIT_ID 311

// TODO this is machine-dependent. Read these values from tracefs instead.
// Raspi
// #define WAKEUP_ID 62
// #define WAKEUP_NEW_ID 61
// #define SWITCH_ID 60
// #define EXIT_ID 57

enum event_type {
	WAKEUP,
	SWITCH,
	EXIT,
};

struct rbftrace_event {
	enum event_type 	typ;
	unsigned long long 	ts;

	/* Common fields. If sched_switch, this information refers to the prev process */
	pid_t 	pid;
	int 	prio;

	/* sched_switch only */
	long 	prev_state; // Current state of the previous process
	pid_t 	next_pid;
	int 	next_prio;

	/* sched_wakeup only*/
	int 	success;
	int 	target_cpu;
};

/* Data of recorder threads */
struct recorder_data {
	int			pid;
	int			event_pipe[2];
	int			cpu;
	int			closed;
	struct tracecmd_input	*stream;
	struct tep_record	*record;
};

/* Record */
struct recorder_data *create_recorders(struct tracefs_instance *tracefs, int cpu_cnt);
int create_recorder(int cpu, int *event_pipe, char *tracefs_path);
void stop_threads(struct recorder_data *recorders, int cpu_cnt);
void wait_threads(struct recorder_data *recorders, int cpu_cnt);

/* Stream */
int parse_event_rbftrace(struct tep_event *source, struct rbftrace_event *target, struct tep_record *record);
struct tracecmd_input *init_stream(int read_fd, int cpu, int cpu_cnt);
int read_stream(struct recorder_data *recorders, int cpu_cnt, struct rbftrace_event *rbf_event);
void print_event(struct rbftrace_event *event);
