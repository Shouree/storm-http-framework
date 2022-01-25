struct elem {
	struct elem *next;
	int value;
};

struct stack {
	struct elem *storage;
	struct elem *queue_head;
	struct elem *free_head;
};

struct stack *stack_init(int elements) {
	struct stack *s = malloc(sizeof(struct stack));
	s->storage = malloc(sizeof(struct elem)*elements);
	s->queue_head = NULL;

	for (int i = 0; i < elements - 1; i++)
		s->storage[i].next = &s->storage[i + 1];

	s->free_head = s->storage;
	return s;
}

static struct elem *pop_internal(struct elem **head) {
	struct elem *result;
	do {
		result = atomic_read(head);
		if (!result)
			return result;

	} while (compare_and_swap(head, result, result->next) != result);
	return result;
}

static void push_internal(struct elem **head, struct elem *push) {
	struct elem *result;
	do {
		result = atomic_read(head);
		push->next = result;
	} while (compare_and_swap(head, result, push) != result);
}

void stack_push(struct stack *s, int value) {
	struct elem *elem = pop_internal(&s->free_head);
	if (elem) {
		elem->value = value;
		push_internal(&s->queue_head, elem);
	}
}

int stack_pop(struct stack *s) {
	struct elem *elem = pop_internal(&s->queue_head);
	if (!elem)
		return -1;

	int r = elem->value;
	push_internal(&s->free_head, elem);
	return r;
}

void thread_fn(struct stack *stack) {
	stack_push(stack, 3);
	stack_push(stack, 4);

	stack_pop(stack);
	stack_pop(stack);
}

int main(void) {
	struct stack *stack = stack_init(6);

	stack_push(stack, 1);
	stack_push(stack, 2);

	thread_new(&thread_fn, stack);

	stack_push(stack, 5);
	stack_push(stack, 6);

	stack_pop(stack);
	stack_pop(stack);

	return 0;
}
