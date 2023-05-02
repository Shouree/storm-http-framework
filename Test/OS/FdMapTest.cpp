#include "stdafx.h"
#include "OS/FdMap.h"
#include <cstdlib>

#ifdef POSIX

BEGIN_TEST(FdMapTest, OS) {
	const int maxFds = 20;
	const int numInserts = 20;
	const int numRemoves = 10;

	os::FdMap<int, 1> fds;
	int vals[maxFds] = { 0 };

	srand(100);

	// Insert some elements:
	for (Nat i = 0; i < numInserts; i++) {
		int insert = rand() % maxFds;
		vals[insert]++;
		fds.put(insert, 0, &vals[insert]);

		fds.dbg_verify();
	}

	// fds.dbg_print();

	// Repeat some times to cause issues.
	for (Nat cycle = 0; cycle < 5; cycle++) {

		// Make sure all elements are reachable.
		for (Nat i = 0; i < maxFds; i++) {
			int count = 0;
			for (nat slot = fds.find(i); slot < fds.capacity(); slot = fds.next(slot)) {
				CHECK_EQ(fds.valueAt(slot), vals + i);
				count++;
			}

			CHECK_EQ(vals[i], count);
		}

		// Now, remove some elements.
		for (Nat i = 0; i < numRemoves; i++) {
			int remove = rand() % maxFds;
			if (vals[remove] == 0)
				continue;

			int ordinal = rand() % vals[remove];

			nat slot = fds.find(remove);
			for (int i = 0; i < ordinal; i++)
				slot = fds.next(slot);

			// PLN(L"Removing slot " << slot);
			// fds.dbg_print();

			fds.remove(slot);
			vals[remove]--;

			VERIFY(fds.dbg_verify());
		}

		// Insert them again.
		for (Nat i = 0; i < numInserts; i++) {
			int insert = rand() % maxFds;
			vals[insert]++;
			// PLN(L"Inserting " << insert);
			// fds.dbg_print();
			fds.put(insert, 0, &vals[insert]);

			VERIFY(fds.dbg_verify());
		}

	}

} END_TEST

#endif
