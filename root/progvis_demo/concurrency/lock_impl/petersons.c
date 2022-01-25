struct lock {
	bool *want_in;
	int turn;
};

void init(struct lock *lock) NO_STEP {
	lock->want_in = malloc(sizeof(bool) * 2);
	lock->turn = 0;
}

int other(int id) NO_STEP {
	if (id == 0)
		return 1;
	else
		return 0;
}

void lock(struct lock *lock, int id) {
	int other = other(id);

	lock->want_in[id] = true;
	lock->turn = other;

	while (lock->want_in[other]
		&& lock->turn == other)
		;
}

void unlock(struct lock *lock, int id) {
	lock->want_in[id] = false;
}

void foo(struct lock *lock, int id) {
	for (int i = 0; i < 5; i++) {
		lock(lock, id);
		// Critical section.
		unlock(lock, id);
	}
}

struct lock l;

int main(void) {
	init(&l);

	thread_new(&foo, &l, 1);
	foo(&l, 0);

	return 0;
}
