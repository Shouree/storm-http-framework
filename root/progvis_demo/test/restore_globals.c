int global_int;
struct lock l;

int main() {
	int local;
	global_int = 30;
	lock_init(&l);
	// comment
	return 0;
}
