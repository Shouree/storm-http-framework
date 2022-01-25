struct my_type {
	int a;
	int b;
	int c;

	my_type() : a(1), b(2), c(3) {}
	~my_type() { a = 0; }
};

void in_array() {
	int *x = new int[2];
	int *end = x + 10;
	int i = 0;
	for (int *at = x; at < end; at++) {
		*at = i++;
	}

	delete []x;
}

void in_complex_array() {
	my_type y[3];
	my_type *x = new my_type[3];

	y[1].a = 8;
	x[1].a = 7;

	delete []x;
}

void in_type() {
	my_type x;

	for (int *a = &x.a; a <= &x.c; ++a) {
		(*a)++;
	}

	print(&x.c - &x.a);
	print(sizeof(my_type));
	print(sizeof(my_type *));
}

int main() {
	int *data = malloc(2*sizeof(int));

	// in_array();
	in_complex_array();

	in_type();

	return 0;
}
