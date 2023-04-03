int suml;
bool has_suml;
int sumh;
bool has_sumh;

void sum_low(int *p) {
	for (int i = 0; i < 2; i++)
		suml += p[i];
	has_suml = true;
}

void sum_high(int *p) {
	for (int i = 2; i < 4; i++)
		sumh += p[i];
	has_sumh = true;
}

int main(void) {
	int *data = malloc(sizeof(int) * 4);
	// Initialization.
	NO_STEP {
		for (int i = 0; i < 4; i++)
			data[i] = 1;
	}

	thread_new(&sum_low, data);
	thread_new(&sum_high, data);

	while (!has_suml)
		;
	while (!has_sumh)
		;

	int sum = suml + sumh;

	printf("Sum: %d\n", sum);
	assert(sum == 4);

	return 0;
}
