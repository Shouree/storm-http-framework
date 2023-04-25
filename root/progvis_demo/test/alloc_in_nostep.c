struct lock {
	bool busy;
};

void lock(struct lock *l) {
	while (atomic_swap(&l->busy, true) == true)
		;
}

void unlock(struct lock *l) {
	atomic_write(&l->busy, false);
}

void work(struct lock *l) {
	while (true) {
		lock(l);
		unlock(l);
	}
}

int main(void) {
	struct lock *l = malloc(sizeof(struct lock));
	NO_STEP {
		thread_new(&work, l);
		work(l);
	}

	return 0;
}
