/**
 * Bounded buffer synchronized with two condition variables.
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

	// Lock for 'rpos', 'wpos' and 'free'.
	struct lock lock;

	// Condition variables, signaled when the queue is not empty or not full.
	struct condition cond_not_empty;
	struct condition cond_not_full;
};

struct buffer *buffer_create(int count) NO_STEP {
	struct buffer *b = malloc(sizeof(struct buffer));
	b->data_begin = malloc(sizeof(int) * count);
	b->data_end = b->data_begin + count;
	b->rpos = b->data_begin;
	b->wpos = b->data_begin;
	b->free = count;
	lock_init(&b->lock);
	cond_init(&b->cond_not_empty);
	cond_init(&b->cond_not_full);
	return b;
}

void buffer_destroy(struct buffer *b) NO_STEP {
	free(b->data_begin);
	lock_destroy(&b->lock);
	cond_destroy(&b->cond_not_empty);
	cond_destroy(&b->cond_not_full);
	free(b);
}

void buffer_put(struct buffer *b, int value) {
	int count = b->data_end - b->data_begin;

	lock_acquire(&b->lock);
	while (b->free == 0)
		cond_wait(&b->cond_not_full, &b->lock);

	*b->wpos = value;
	if (++b->wpos == b->data_end)
		b->wpos = b->data_begin;

	if (b->free-- == count)
		cond_signal(&b->cond_not_empty, &b->lock);
	lock_release(&b->lock);
}

int buffer_get(struct buffer *b) {
	int count = b->data_end - b->data_begin;

	lock_acquire(&b->lock);
	while (b->free == count)
		cond_wait(&b->cond_not_empty, &b->lock);

	int r = *b->rpos;
	if (++b->rpos == b->data_end)
		b->rpos = b->data_begin;

	if (b->free++ == 0)
		cond_signal(&b->cond_not_full, &b->lock);
	lock_release(&b->lock);

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
