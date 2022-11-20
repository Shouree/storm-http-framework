struct lock l;
bool a;
bool b;

void worker(void) {
	lock_acquire(&l);
	if (!b) {
		a = true;
	}
	lock_release(&l);
}

int main(void) {
	lock_init(&l);
	thread_new(&worker);

	bool done = false;
	while (!done) {
		lock_acquire(&l);
		b = true;
		done = a;
		lock_release(&l);
	}

	return 0;
}
