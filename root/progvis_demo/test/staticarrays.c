int intarray[4];
int intarray2[2] = { 1, 2 };
struct lock lockarray[4];
int globalinit = 3;

int main() {
	int z = 2;
	int localdynarray[z]; // = { 1, 2 };
	int localstaticarray[1 + 1] = { 1, 2 };
	int localarray[] = { 1, 2, 3 };
	return 0;
}
