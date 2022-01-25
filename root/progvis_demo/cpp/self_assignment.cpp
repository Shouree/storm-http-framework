#include <cstdio>

class shared_data {
public:
	// Number of references to this data.
	int references;

	// The actual data.
	int data;
};

class shared_ptr {
public:
	shared_ptr(int data) : shared{new shared_data} {
		shared->references = 1;
		shared->data = data;
	}

	~shared_ptr() {
		if (--shared->references == 0) {
			delete shared;
		}
	}

	shared_ptr(const shared_ptr &other) : shared{other.shared} {
		shared->references++;
	}

	// Note: This does intentionally not work for self-assignment.
	shared_ptr &operator =(const shared_ptr &other) {
		if (--shared->references == 0) {
			delete shared;
		}

		shared = other.shared;
		shared->references++;

		return *this;
	}

private:
	shared_data *shared;
};

// Helper function to show that it is not obvious when self-assignment happens.
void helper(shared_ptr &a, shared_ptr &b) {
	a = b;
	b = shared_ptr(9);
}

int main() {
	int value = 5;
	shared_ptr a(value);
	helper(a, a);

	return 0;
}
