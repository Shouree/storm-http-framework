#include <cstdio>
#include <cstring>

class Base {
public:
	size_t data;

	virtual ~Base() {}

	void regular() {
		printf("Regular Base\n");
	}

	virtual void dynamic() {
		printf("Dynamic Base\n");
	}

	virtual void dynamic2() {
		printf("Dynamic2 Base\n");
	}
};

class Derived : public Base {
public:
	void regular() {
		printf("Regular Derived\n");
	}

	virtual void dynamic() {
		printf("Dynamic Derived\n");
	}

	virtual void dynamic2() {
		printf("Dynamic2 Derived\n");
	}
};

void foo() {
	Base base;
}
