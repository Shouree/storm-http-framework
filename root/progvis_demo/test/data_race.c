int shared;
int other;
struct semaphore sema;

// Rules:
// - release clears reads/writes
// - acquire clears (I don't think it needs to).
// - actual calls to Progvis can happen after the call

void fn_a() {
	other = 20;
	shared = 10;
	sema_up(&sema); // release
	int tmp = other; // may be moved earlier
}

void fn_b() {
	other = 20; // may be moved later
	sema_down(&sema); // acquire
	int tmp = shared;
}

void fn_c() {
	int tmp = shared;
}

int main(void) {
	sema_init(&sema, 0);

	other = 30;

	thread_new(&fn_a);
	thread_new(&fn_b);
	thread_new(&fn_c);

	return 0;
}
