int main() {
	char buffer[4];
	// This should work.
	const char *p = buffer;

	const char *str = "abcd";
	const char *stri = &str[8];
	const char *buf[9];
	const char **bufi;
	const char **bufend;
	bufi = buf;
	bufend = &buf[9];
	while (bufi != bufend) {
		*bufi = stri;
		bufi++;
		stri--;
	}

	do {
		bufi--;
		// This should trigger an error!
		*(*bufi) -= 32;
	} while (bufi != buf);

	while (bufi != bufend){
		putchar(**bufi);
		bufi++;
	}

	return 0;
}
