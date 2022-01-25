struct lock {
	bool busy;
};

void init(struct lock *lock) {
	lock->busy = false;
}

void lock(struct lock *lock) {
	while (lock->busy)
		;
	lock->busy = true;
}

void unlock(struct lock *lock) {
	lock->busy = false;
}

void foo(struct lock *lock) {
	for (int i = 0; i < 5; i++) {
		lock(lock);
		// Critical section.
		unlock(lock);
	}
}

struct lock l;

int main(void) {
	init(&l);

	thread_new(&foo, &l);
	foo(&l);

	return 0;
}
