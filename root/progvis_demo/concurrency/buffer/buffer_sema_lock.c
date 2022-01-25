/**
 * Version synchronized with a pair of semaphores and locks.
 *
 * This makes the implementation entirely threadsafe, even when multiple threads call "get" and "put".
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

	// Lock for 'rpos'.
	struct lock rlock;

	// Lock for 'wpos'.
	struct lock wlock;
};

struct buffer *buffer_create(int count) NO_STEP {
	struct buffer *b = malloc(sizeof(struct buffer));
	b->data_begin = malloc(sizeof(int) * count);
	b->data_end = b->data_begin + count;
	b->rpos = b->data_begin;
	b->wpos = b->data_begin;
	sema_init(&b->free, count);
	sema_init(&b->filled, 0);
	lock_init(&b->rlock);
	lock_init(&b->wlock);
	return b;
}

void buffer_destroy(struct buffer *b) NO_STEP {
	free(b->data_begin);
	sema_destroy(&b->free);
	sema_destroy(&b->filled);
	lock_destroy(&b->rlock);
	lock_destroy(&b->wlock);
	free(b);
}

void buffer_put(struct buffer *b, int value) {
	sema_down(&b->free);

	lock_acquire(&b->wlock);
	*b->wpos = value;
	if (++b->wpos == b->data_end)
		b->wpos = b->data_begin;
	lock_release(&b->wlock);

	sema_up(&b->filled);
}

int buffer_get(struct buffer *b) {
	sema_down(&b->filled);

	lock_acquire(&b->rlock);
	int r = *b->rpos;
	if (++b->rpos == b->data_end)
		b->rpos = b->data_begin;
	lock_release(&b->rlock);

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
	thread_new(&write, b);
	// thread_new(&read, b);

	thread_new(&write, b);
	read(b);

	buffer_destroy(b);

	return 0;
}
