#include <stdio.h>

int fibonacci(int x) {
	int result = 0;
	if (x <= 1) {
		result = x;
	} else {
		result = fibonacci(x - 1);
		result += fibonacci(x - 2);
	}
	return result;
}

int main(void) {
	int x = 3;
	int fib = fibonacci(3);
	printf("fibonacci(%d) = %d\n", x, fib);

	return 0;
}
