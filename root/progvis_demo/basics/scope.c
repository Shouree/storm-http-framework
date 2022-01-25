#include <stdio.h>

int i;

void function(void) {
	int i = 1;
	printf("I is %d\n", i);
	i = i + 1;
	printf("I is %d\n", i);
}

int main(void) {
	printf("I is %d\n", i);
	function();
	printf("I is %d\n", i);

	return 0;
}
