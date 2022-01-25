#include <stdio.h>

struct data {
	int a;
	int b;
};

int main(void) {
	struct data *array = malloc(sizeof(struct data) * 3);
	void *x = array;
	array[0].a = 2;
	array[1].a = 3;
	(array + 2)->a = 4;

	{
		struct data *p = array;
		p->a = 1;
		p++;
		p->b = 2;
	}
	{
		int *q = &array->a;
		*q = 10;
		q++;
		*q = 11;
		q++;
		*q = 12;
	}

	return 0;
}
