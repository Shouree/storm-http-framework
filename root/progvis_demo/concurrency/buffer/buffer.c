/**
 * Original, un-synchronized version of a bounded buffer.
 */


struct buffer {
	// Array of data elements.
	int *data_begin;

	// Last position in the 'data' array.
	int *data_end;

	// Read position.
	int *rpos;

	// Write position.
	int *wpos;

	// Number of free elements.
	int free;
};

struct buffer *buffer_create(int count) NO_STEP {
	struct buffer *b = malloc(sizeof(struct buffer));
	b->data_begin = malloc(sizeof(int) * count);
	b->data_end = b->data_begin + count;
	b->rpos = b->data_begin;
	b->wpos = b->data_begin;
	b->free = count;
	return b;
}

void buffer_destroy(struct buffer *b) NO_STEP {
	free(b->data_begin);
	free(b);
}

void buffer_put(struct buffer *b, int value) {
	while (b->free == 0)
		;

	*b->wpos = value;
	if (++b->wpos == b->data_end)
		b->wpos = b->data_begin;
	--b->free;
}

int buffer_get(struct buffer *b) {
	int count = b->data_end - b->data_begin;
	while (b->free == count)
		;

	int r = *b->rpos;
	if (++b->rpos == b->data_end)
		b->rpos = b->data_begin;
	++b->free;
	return r;
}

// Write some elements to a buffer.
void write(struct buffer *to) {
	for (int i = 0; i < 5; i++)
		buffer_put(to, i);
}

// Read some elements from a buffer.
void read(struct buffer *from) {
	for (int i = 0; i < 5; i++) {
		int v = buffer_get(from);
		printf("Got %d\n", v);
	}
}

int main(void) {
	struct buffer *b = buffer_create(3);

	// Try with multiple readers and writers:
	// thread_new(&write, b);
	// thread_new(&read, b);

	thread_new(&write, b);
	read(b);

	buffer_destroy(b);

	return 0;
}
