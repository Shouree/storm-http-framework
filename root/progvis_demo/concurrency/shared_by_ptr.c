void thread(int *data) {
	*data = 10;
}

int main(void) {
	int data;

	thread_new(&thread, &data);

	printf("Data: %d\n", data);

	return 0;
}
