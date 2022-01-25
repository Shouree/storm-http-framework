struct element {
	int value;
	struct element *next;
};

struct queue {
	struct element *head;
	struct lock lock;
};

void push(struct queue *to, int value) {
	struct element *next = malloc(sizeof(struct element));
	lock_acquire(&to->lock);
	next->next = to->head;
	lock_release(&to->lock);

	next->value = value;

	lock_acquire(&to->lock);
	to->head = next;
	lock_release(&to->lock);
}

void thread_fn(struct queue *to) {
	push(to, 8);
}

int main(void) {
	struct queue *q;
	NO_STEP {
		q = malloc(sizeof(struct queue));
		lock_init(&q->lock);
		q->head = malloc(sizeof(struct element));
	}

	thread_new(&thread_fn, q);
	push(q, 10);
	return 0;
}
