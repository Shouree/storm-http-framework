struct elem {
	elem *next;
	int value;
};

elem *createList(int count) {
	elem *head;
	if (count <= 0)
		return head;

	head = new elem;
	head->value = 1;
	elem *tail = head;
	while (tail->value < count) {
		elem *tmp = new elem;
		tail->next = tmp;
		tmp->value = tail->value + 1;
		tail = tmp;
	}

	return head;
}

void printList(elem *list) {
	if (!list)
		return;

	for (elem *at = list; at != nullptr; at = at->next) {
		print(at->value);
	}
}

int main() {
	int test = 1 + 1, 2 + 2, 3 + 3;
	bool a = false && true;
	bool b = false || true;
	if (b)
		if (!b)
			print(1);
		else
			print(2);

	int x = 0;
	do {
		x++;
	} while (x < 5);

	elem *list = createList(4);
	if (list) {
		printList(list);
	}

	return 0;
}
