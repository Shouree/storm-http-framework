struct fork {
	int id;
	struct semaphore sema;
};

struct fork **make_forks(int num) NO_STEP {
	struct fork **r = malloc(sizeof(struct fork *) * num);
	for (int i = 0; i < num; i++) {
		struct fork *f = malloc(sizeof(struct fork));
		f->id = i;
		sema_init(&f->sema, 1);
		r[i] = f;
	}
	return r;
}

void philosopher(struct fork *left, struct fork *right) {
	if (left->id < right->id) {
		sema_down(&left->sema);
		sema_down(&right->sema);
	} else {
		sema_down(&right->sema);
		sema_down(&left->sema);
	}

	// Eat...

	sema_up(&left->sema);
	sema_up(&right->sema);
}


int main(void) {
	struct fork **forks = make_forks(3);

	thread_new(&philosopher, forks[0], forks[1]);
	thread_new(&philosopher, forks[1], forks[2]);
	thread_new(&philosopher, forks[2], forks[0]);

	free(forks);

	return 0;
}
