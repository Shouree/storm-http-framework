#include <stdio.h>

int main(void) {
	int *data = malloc(sizeof(int) * 5);
	data[0] = 1;
	data[3] = 2;

	int *ptr = data + 3;
	ptr[1] = 5;

	return 0;
}
