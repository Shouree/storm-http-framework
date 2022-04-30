int counter;
struct lock a;
struct lock b;

void thread_fn(void) {
	lock_acquire(&b);

	lock_acquire(&a);
	lock_release(&b); // Progvis failed to stop here in some situations.
	counter++;
	lock_release(&a);
}

int main(void) {
	lock_init(&a);
	lock_init(&b);

	thread_new(&thread_fn);
	lock_acquire(&a);
	lock_release(&a);

	return 0;
}
