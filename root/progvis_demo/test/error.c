#include "wrap/thread.h"
#include "wrap/synch.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int do_work(int param);

struct running_thread {
	int param;

	int result;

	struct semaphore *sema;
};

void thread_main(struct running_thread *data) {
	data->result = do_work(data->param);
	sema_up(data->sema);
}

struct running_thread *exec(int param) {
	struct running_thread *data = malloc(sizeof(struct running_thread));
	data->param = param;

	thread_new(&thread_main, data);
	data->sema = malloc(sizeof(struct semaphore));
	sema_init(data->sema, 0);

	return data;
}

int wait(struct running_thread *data) {
	sema_down(data->sema);
	int result = data->result;
	free(data);
	return result;
}

int do_work(int param) {
	timer_msleep(param);

	return param * param;
}

int main(void) {
	struct running_thread *a = exec(10);
	struct running_thread *b = exec(100);

	int c = do_work(5);

	printf("Result for 'a': %d\n", wait(a));
	printf("Result for 'b': %d\n", wait(b));
	printf("Result for 'c': %d\n", c);

	return 0;
}
