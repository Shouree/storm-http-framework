#include <stdlib.h>

int leak(void) {
	int *ptr = malloc(sizeof(int));
	*ptr = 20;
	return *ptr;
}

int main(void) {
	int result = leak();

	return 0;
}
