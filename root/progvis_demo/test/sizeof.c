int intarray[] = { 1, 2, 3 };
int iarray2[4] = { 1, 2 };

struct mytype {
	int w;
	int x;
};

int main() {
	int sz1 = 3;
	int array1[sz1];

	const int sz2 = 5;
	int array2[sz2] = { 1, 2 };

	printf("Integer: %d\n", sizeof(int));
	printf("Struct: %d\n", sizeof(mytype));
	printf("Struct: %d\n", sizeof(struct mytype));
	printf("Expression: %d\n", sizeof(1 + 5));
	printf("Global: %d\n", sizeof(intarray));
	printf("Local 1: %d\n", sizeof(array1));
	printf("Local 2: %d\n", sizeof(array2));

	// Note: We can't do this on one line at the moment.
	int *x;
	x = malloc(sizeof(*x) * 5);

	return 0;
}
