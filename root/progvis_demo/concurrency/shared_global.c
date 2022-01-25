int global;

void thread(int *dummy) {
	global = 8;
}

int main(void) {
	thread_new(&thread, NULL);

	global = 5;

	return 0;
}
