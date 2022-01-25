struct shared {
	struct lock lock;
	struct condition cond;
};

void worker(struct shared *shared) {
	lock_acquire(&shared->lock);
	cond_signal(&shared->cond, &shared->lock);
	lock_release(&shared->lock);
}

int main(void) {
	struct shared shared;
	lock_init(&shared.lock);
	cond_init(&shared.cond);

	thread_new(&worker, &shared);

	lock_acquire(&shared.lock);
	cond_wait(&shared.cond, &shared.lock);
	lock_release(&shared.lock);

	return 0;
}
