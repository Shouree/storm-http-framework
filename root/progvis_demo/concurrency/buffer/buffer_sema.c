/**
 * Version synchronized with a pair of semaphores.
 *
 * Note, this only works for one thread calling "put" while another is calling "get".
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
	struct semaphore free;

	// Number of filled elements.
	struct semaphore filled;
};

struct buffer *buffer_create(int count) NO_STEP {
	struct buffer *b = malloc(sizeof(struct buffer));
	b->data_begin = malloc(sizeof(int) * count);
	b->data_end = b->data_begin + count;
	b->rpos = b->data_begin;
	b->wpos = b->data_begin;
	sema_init(&b->free, count);
	sema_init(&b->filled, 0);
	return b;
}

void buffer_destroy(struct buffer *b) NO_STEP {
	free(b->data_begin);
	sema_destroy(&b->free);
	sema_destroy(&b->filled);
	free(b);
}

void buffer_put(struct buffer *b, int value) {
	sema_down(&b->free);

	*b->wpos = value;
	if (++b->wpos == b->data_end)
		b->wpos = b->data_begin;

	sema_up(&b->filled);
}

int buffer_get(struct buffer *b) {
	sema_down(&b->filled);

	int r = *b->rpos;
	if (++b->rpos == b->data_end)
		b->rpos = b->data_begin;

	sema_up(&b->free);
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
