int intarray[4];
int intarray2[2] = { 1, 2 };
int intarray3[] = { 1, 2, 3, 4, 5 };
struct lock lockarray[4];
int globalinit = 3;

int main() {
	int z = 2;
	int localdynarray[z]; // = { 1, 2 };
	int localstaticarray[1 + 1] = { 1, 2 };
	int localarray[] = { 1, 2, 3 };

	struct lock *lck = lockarray + 1;
	lock_init(&lockarray[1]);
	return 0;
}
