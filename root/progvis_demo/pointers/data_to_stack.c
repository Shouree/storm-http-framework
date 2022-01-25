struct data {
	int *ptr;
};

int main(void) {
	int local;
	struct data *d = malloc(sizeof(struct data));
	d->ptr = &local;

	return 0;
}
