int main() {
	// Allocate on the heap to not flood the stack with data.
	char *str = strcpy("sihtgubed");
	char *stri = &str[8];
	char *buf[9];
	char **bufi;
	char **bufend;
	bufi = buf;
	bufend = &buf[9];
	while (bufi != bufend) {
		*bufi = stri;
		bufi++;
		stri--;
	}

	do {
		bufi--;
		*(*bufi) -= 32;
	} while (bufi != buf);

	while (bufi != bufend){
		putchar(**bufi);
		bufi++;
	}

	putchar('\n');
	putstr("Done!");

	return 0;
}
