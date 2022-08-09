#include "stdafx.h"
#include "OS/StackCall.h"

static int *data;

int onStack(int val) {
	int z = val;
	// Note: We need to do an atomic write here, as this is technically undefined behavior.
	atomicWrite(data, &z);
	return z + 10;
}

void stackThrow(int on) {
	if (on > 0) {
		throw on;
	}
}

BEGIN_TEST(StackCall, OS) {
	onStack(100);

	os::Stack s(1024*1024); // Note: ARM64 needs more than 1K of stack for proper exception handling.

	int src = 42;
	os::FnCall<int, 2> call = os::fnCall().add(src);
	int result = os::stackCall(s, address(&onStack), call, false);

	CHECK_EQ(result, 52);
	CHECK_EQ(*data, 42);
	s.clear();

	// "data" should be on the allocated stack somewhere.
	CHECK_GT(size_t(data), size_t(s.low()));
	CHECK_LT(size_t(data), size_t(s.high()));

	// Check exceptions.
	src = -1;
	os::FnCall<void, 2> second = os::fnCall().add(src);
	os::stackCall(s, address(&stackThrow), second, false);

#ifndef WINDOWS

	// Note: Does not work on Win32, and is currently not needed there. It is used for a workaround
	// in the Gui library on Linux.
	src = 1;
	try {
		os::stackCall(s, address(&stackThrow), second, false);
		CHECK(false);
	} catch (int i) {
		CHECK_EQ(i, 1);
	}

#endif

	CHECK_EQ(os::UThread::current().threadData()->stack.detourTo, (void *)null);

} END_TEST
