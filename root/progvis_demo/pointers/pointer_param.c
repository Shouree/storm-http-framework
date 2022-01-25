#include <stdio.h>

void increment(int *x) {
	*x = *x + 1;
}

int main(void) {
	int a = 1;
	increment(&a);
	printf("a = %d\n", a);

	return 0;
}
