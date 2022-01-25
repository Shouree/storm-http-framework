#include <stdio.h>

int main(void) {
	int data[5] = { 1, 2, 3, 4, 5 };
	data[2] = 8;

	int *iter = data;
	*iter = 2;
	iter++;
	*iter = 3;

	*(iter + 3) = 9;
	iter[3] = 8;

	return 0;
}
