int suml;
int sumh;
struct semaphore sema;

void sum_low(int *p) {
	for (int i = 0; i < 2; i++)
		suml += p[i];
	sema_up(&sema);
}

void sum_high(int *p) {
	for (int i = 2; i < 4; i++)
		sumh += p[i];
	sema_up(&sema);
}

int main(void) {
	int *data = malloc(sizeof(int) * 4);
	// Initialization.
	NO_STEP {
		for (int i = 0; i < 4; i++)
			data[i] = 1;
	}

	sema_init(&sema, 2);

	thread_new(&sum_low, data);
	thread_new(&sum_high, data);

	sema_down(&sema);

	int sum = suml + sumh;

	printf("Sum: %d\n", sum);
	assert(sum == 4);

	return 0;
}
