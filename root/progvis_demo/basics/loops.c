#include <stdio.h>

int main(void) {
	for (int i = 0; i < 3; i++) {
		printf("First loop: %d\n", i);
	}

	int j = 1;
	do {
		j *= 2;
		printf("Second loop: %d\n", j);
	} while (j < 4);

	return 0;
}
