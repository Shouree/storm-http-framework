int main(void) {
	int a;
	int b;
	bool x;
	bool y;

	b = test_and_set(&a);
	y = test_and_set(&x);

	b = atomic_add(&a, 10);
	b = atomic_sub(&a, 5);

	atomic_read(&b);

	atomic_swap(&a, 8);

	compare_and_swap(&a, 7, 10);
	compare_and_swap(&a, 8, 10);

	const char *strA = "A";
	const char *strB = "B";
	const char *tmp;
	tmp = compare_and_swap(&strA, strB, "Z");
	tmp = compare_and_swap(&strA, tmp, "W");

	tmp = atomic_swap(&strA, "X");


	atomic_read(&strA);
	atomic_write(&strA, tmp);

	return 0;
}
