#include <stdlib.h>

int main(void) {
	int *ptr = malloc(sizeof(int));
	*ptr = 10;

	free(ptr);
	*ptr = 20;

	return 0;
}
