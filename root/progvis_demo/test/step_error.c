struct data {
	int a;
	int b;
};

void thread_main(struct data *d) {
	int temp = d->a;
	temp += d->b;
	d->b = temp;
}

int main(void) {
	struct data *d = malloc(sizeof(struct data));
	d->a = 10;
	d->b = 20;
	thread_new(&thread_main, d);
	// Note: This is skipped for some reason...
	thread_main(d);
	return 0;
}
