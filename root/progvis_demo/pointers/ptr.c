struct data {
	int a;
	int *b;
};

int main(void) {
	struct data *array
		= malloc(sizeof(struct data)*3);
	struct data *p = array + 1;
	int *p_a = &p->a;
	int **p_b = &p->b;
	*p_b = &array[2].a;
	array[2].b = malloc(sizeof(int)*2);
	printf("%d\n", array[1].a);

	return 0;
}
