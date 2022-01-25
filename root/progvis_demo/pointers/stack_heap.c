#include <stdlib.h>

int main(void) {
	int stack = 0;

	int *ptr_stack = &stack;
	int *ptr_heap = malloc(sizeof(int));

	*ptr_stack = 10;
	*ptr_heap = 20;

	free(ptr_heap);

	return 0;
}
