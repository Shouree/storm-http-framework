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

my_class return_local(int a) {
	my_class tmp{a, a + 1};
	return tmp;
}

int main() {
	my_class a{return_local(1)};
	a = return_local(2);
	return 0;
}
