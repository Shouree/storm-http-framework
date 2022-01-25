#include <stdio.h>
#include "live6.h"

struct sorted_list {
	int *data;
	int size;
	int filled;
};

void list_create(struct sorted_list *me, int max) {
	me->data = malloc(sizeof(int)*max);
	me->size = max;
	me->filled = 0;
}

void list_destroy(struct sorted_list *me) {
	free(me->data);
}

bool list_done(const struct sorted_list *me, int *pos, int value) {
	if (pos == me->data)
		return true;
	return value >= *(--pos);
}

bool list_add(struct sorted_list *me, int value) {
	if (me->filled >= me->size)
		return false;

	int *into = me->data + me->filled++;
	while (!list_done(me, into, value)) {
		--into;
		*(into + 1) = *into;
	}

	*into = value;
	return true;
}

int list_size(const struct sorted_list *me) {
	return me->filled;
}

int list_get(const struct sorted_list *me, int pos) {
	if (pos >= me->filled || pos < 0) {
		// If we know more about the data, we could return
		// some invalid value, eg. NULL.
		printf("out of range: %d\n", pos);
		// abort();
	}

	return me->data[pos];
}

void print_first(int count, const struct sorted_list *l) {
	printf("De forsta %d elementen:\n", count);
	for (int i = 0; i < count; i++) {
		printf("%3d", list_get(l, i));
		if (i % 5 == 4)
			printf("\n");
	}
}

int main() {
	struct sorted_list l;
	list_create(&l, 10);

	for (int i = 0; i < 10; i++)
		list_add(&l, 10 - i);

	printf("Antal element: %d\n", list_size(&l));
	print_first(10, &l);

	list_destroy(&l);

	return 0;
}
