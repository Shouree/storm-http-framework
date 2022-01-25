#include <cstdio>

class my_class {
public:
	my_class(int value) : data{new int} {
		*data = value;
	}

	my_class(int *data) : data{data} {}

	~my_class() {
		delete data;
	}

	my_class(const my_class &other) : data{new int} {
		*data = *other.data;
	}

	my_class &operator =(const my_class &other) {
		*data = *other.data;
		return *this;
	}

	my_class(my_class &&other) : data{other.data} {
		other.data = nullptr;
	}

	my_class &operator =(my_class &&other) {
		delete data;
		data = other.data;
		other.data = nullptr;
		return *this;
	}

private:
	int *data;
};

my_class do_move() {
	my_class c(8);
	return c;
}

int main() {
	my_class original(10);

	// Copy constructor.
	my_class copy(original);

	// Assign operator.
	original = copy;

	// Move assignment/move constructor.
	my_class other = do_move();

	return 0;
}
