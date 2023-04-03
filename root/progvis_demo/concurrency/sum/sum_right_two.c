int suml;
int sumh;
struct semaphore has_suml;
struct semaphore has_sumh;

void sum_low(int *p) {
	for (int i = 0; i < 2; i++)
		suml += p[i];
	sema_up(&has_suml);
}

void sum_high(int *p) {
	for (int i = 2; i < 4; i++)
		sumh += p[i];
	sema_up(&has_sumh);
}

int main(void) {
	int *data = malloc(sizeof(int) * 4);
	// Initialization.
	NO_STEP {
		for (int i = 0; i < 4; i++)
			data[i] = 1;
	}

	sema_init(&has_suml, 0);
	sema_init(&has_sumh, 0);

	thread_new(&sum_low, data);
	thread_new(&sum_high, data);

	sema_down(&has_suml);
	sema_down(&has_sumh);

	int sum = suml + sumh;

	printf("Sum: %d\n", sum);
	assert(sum == 4);

	return 0;
}
