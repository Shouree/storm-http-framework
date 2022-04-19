int main(void) {
	struct lock lock;
	lock_init(&lock);
	lock_acquire(&lock);
	// lock_acquire(&lock);
	lock_release(&lock);

	struct semaphore sema;
	sema_init(&sema, 1);
	sema_down(&sema);
	// sema_down(&sema);
	sema_up(&sema);
	return 0;
}
