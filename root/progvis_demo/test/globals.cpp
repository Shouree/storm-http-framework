struct data {
	int a;
	int b;
};

struct data global_data;
int global_int;


int main() {
	global_data.a = 10;
	global_data.b = 20;

	global_int = 30;

	struct data *p = &global_data;
	p->a++;

	return 0;
}
