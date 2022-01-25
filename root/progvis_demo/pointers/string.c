#include <stdio.h>

void print(const char *s) {
	for (const char *i = s; *i; i++) {
		putchar(*i);
	}
	putchar('\n');
}

int main(void) {
	const char *s = "Hello!";
	print(s);

	char a[] = "Hi!";
	print(a);

	return 0;
}
