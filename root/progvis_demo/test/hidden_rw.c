int global;
int count;
struct lock l;
struct semaphore sema;

void do_access(void) {
	lock_acquire(&l);
	int val = count--;
	lock_release(&l);

	if (val == 0) {
		global = 1;
	}
}

// This function is problematic (in large-scale problems) since the state when both threads are
// about to execute "sema_up" contains hidden state. At that point, the model checker knows that
// some thread has written to "count", but not which of the two threads. That state is no longer
// visible here. For this reason, we need to store per-thread accesses in the summaries.
void fn(void) {
	do_access();
	// For a barrier.
	sema_up(&sema);
}

int main(void) {
	sema_init(&sema, 0);
	lock_init(&l);
	count = 1;

	thread_new(fn);
	fn();
	return 0;
}
