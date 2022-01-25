void test() {
	const char *str = "Hello!";
	char *to = new char[3];
	for (int i = 0; str[i]; i++)
		to[i] = str[i];
}

int main() {
	test();
	return 0;
}
