#include <stdio.h>

int main(void) {
	int a = 10;
	int *b = &a;
	int **c = &b;

	*b = 11;
	printf("a = %d\n", a);

	**c = 12;
	printf("a = %d\n", a);

	return 0;
}
