class my_class {
public:
	my_class(int a, int b) : a(a), b(b) {}
	my_class(const my_class &other) : a(other.a), b(other.b) {}
	my_class(my_class &&other) : a(other.a), b(other.b) {
		other.a = 0;
		other.b = 0;
	}

	my_class &operator =(const my_class &other) {
		a = other.a;
		b = other.b;
		return *this;
	}

	my_class &operator =(my_class &&other) {
		a = other.a;
		b = other.b;
		other.a = 0;
		other.b = 0;
		return *this;
	}

	int a;
	int b;
};

int main() {
	my_class a{1, 2};
	my_class b{my_class{3, 4}};

	my_class c{a};
	c = b;
	c = my_class{9, 1};

	return 0;
}
