#include <stdio.h>

struct data {
	int a;
	int b;
	int c;
};

int main(void) {
	struct data d;
	d.a = 5;

	{
		struct data *p = &d;
		(*p).b = 6;
		p->c = 7;
	}
	{
		int *q = &d.a;
		*q = 6;

		q = &d.b;
		*q = 7;
	}

	return 0;
}
