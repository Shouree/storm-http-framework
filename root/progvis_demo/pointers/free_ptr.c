int main(void) {
	int *x;
	NO_STEP {
		int *r = malloc(sizeof(int) * 3);
		x = r + 1;
		free(r);
	}

	// We previously had a bug that failed to recognize the allocation as an array allocation in
	// this case, i.e. when the allocation was only seen after it was freed.

	*x = 10;

	return 0;
}
