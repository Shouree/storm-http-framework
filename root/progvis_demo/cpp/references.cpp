#include <cstdio>

struct my_data {
	int a;
	int b;

	my_data(int a, int b) : a{a}, b{b} {}
};

my_data &my_function(int &value) {
	value += 10;
	my_data x(value, 10);
	return x;
}

int main() {
	int value = 20;
	my_data &data = my_function(value);

	data.b = 8;

	return 0;
}
