#include "stdafx.h"
#include "Utils/Bitwise.h"


static void *allocCode(Gc &gc, size_t size, size_t refs, nat pattern) {
	void *r = gc.allocCode(size, refs);
	memset(r, pattern & 0xFF, size);
	return r;
}

struct TestEntry {
	size_t count;
	size_t size;
	size_t refs;
};

static const TestEntry seq[] = {
	{ 1, 50, 2 },
	{ 58, 20, 3 },
	{ 2, 60, 2 },
	{ 1, 68, 2 },
	{ 2, 20, 3 },
	{ 1, 44, 10 },
	{ 20, 8, 2 },
};

nat align(nat v) {
	return roundUp(v, Nat(sizeof(void *)));
}
size_t align(size_t v) {
	return roundUp(v, sizeof(void *));
}

void verify(const RootArray<void> &data) {
	nat pos = 0;
	for (nat i = 0; i < ARRAY_COUNT(seq); i++) {
		const TestEntry &e = seq[i];
		for (size_t j = 0; j < e.count; j++) {
			byte *d = (byte *)data[pos];
			if (!d)
				return;

			if (Gc::codeSize(d) != align(e.size))
				PLN(pos << L": SIZE CHECK FAILED: is " << Gc::codeSize(d) << L", should be " << align(e.size));
			if (Gc::codeRefs(d)->refCount != e.refs)
				PLN(pos << L": REF COUNT CHECK FAILED!");

			for (nat q = 0; q < e.size; q++) {
				if (d[q] != (pos & 0xFF)) {
					PLN(pos << L": VERIFICATION ERROR IN ALLOC #" << pos);
				}
			}

			pos++;
		}
	}
}

BEGIN_TEST(CodeFmtTest, GcScan) {
	Gc &gc = ::gc();

	RootArray<void> data(gc);
	data.resize(1000);

	// Allocate some objects, verify everything all the time!
	nat pos = 0;
	for (nat i = 0; i < ARRAY_COUNT(seq); i++) {
		const TestEntry &e = seq[i];
		for (size_t j = 0; j < e.count; j++) {
			data[pos] = allocCode(gc, e.size, e.refs, pos);
			pos++;

			verify(data);
		}

		// Try to force a garbage collection.
		gc.test(1);
		// NOTE: Calling 'gc.collect' with a small heap seems to make the GC consume
		// a lot of time during shutdown for some reason. Possibly, a lot of small areas
		// are allocated, which causes the problems.
		// gc.collect();
	}


} END_TEST
