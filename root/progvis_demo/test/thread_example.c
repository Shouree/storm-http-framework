#include "wrap/thread.h"
#include "wrap/sync.h"

struct parameters_to_start {
	const char *start;
	struct semaphore sema;
};

void process_start(struct parameters_to_start *test) {
	for (const char *at = test->start; *at; at++) {
		putchar(*at);
	}
	putchar('\n');

	sema_up(&test->sema);
}

int start_process(const char *name) {
	struct parameters_to_start params;
	params.start = name;
	sema_init(&params.sema, 0);

	int tid = thread_new(process_start, &params);

	sema_down(&params.sema);

	return tid;
}

int main() {
	int pid = start_process("test");
	// TODO: Make some kind of printf.
	print(pid);

	return 0;
}
