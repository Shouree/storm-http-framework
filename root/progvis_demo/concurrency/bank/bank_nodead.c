struct account {
	int balance;
	struct lock lock;
};

int num_accounts;
struct account *accounts;

bool transfer(int amount, struct account *from, struct account *to) {
	if (from < to) {
		lock_acquire(&from->lock);
		lock_acquire(&to->lock);
	} else {
		lock_acquire(&to->lock);
		lock_acquire(&from->lock);
	}

	bool ok = from->balance >= amount;
	if (ok) {
		from->balance -= amount;
		to->balance += amount;
	}

	lock_release(&from->lock);
	lock_release(&to->lock);

	return ok;
}

void worker(int amount, int from, int to) {
	if (!transfer(amount, &accounts[from], &accounts[to]))
		printf("Transfer %d from %d to %d failed!\n", amount, from, to);
}

int main(void) {
	NO_STEP {
		num_accounts = 5;
		accounts = malloc(sizeof(struct account) * num_accounts);
		for (int i = 0; i < num_accounts; i++) {
			accounts[i].balance = 10;
			lock_init(&accounts[i].lock);
		}
	}

	thread_new(&worker, 10, 2, 0);
	worker(10, 0, 2);

	return 0;
}
