class inner {
public:
	int a;
	int b;

	inner() : a(1), b(2) {
		a++;
	}

	inner(int a, int b) : a(a), b(b) {}

	~inner() {
		b = 30;
	}

	int sum() {
		return a + b;
	}
};


class outer {
public:
	int a;
	inner b;

	outer() : a(1) {
		a++;
	}

	outer(int a, int b, int c) : a(a), b(b, c) {}

	~outer() {
		a = 20;
	}

	int sum() {
		return a + b.sum();
	}
};

int main() {
	outer o;
	print(o.sum());

	outer *a = new outer(1, 2, 3);
	a->sum();
	delete a;

	return 0;
}
