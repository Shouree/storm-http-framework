int fn(int x) {
	return x;
}

int main() {
	int v = 10;
	const char *s = "World";
	const char *t = s + 1;
	printf("Hello %10d!\n", fn(v));
	printf("Hello %10s!\n", s);
	printf("Hello %10s!\n", t);

	return 0;
}
