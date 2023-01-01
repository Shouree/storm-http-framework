#include "stdafx.h"
#include "OS/Thread.h"
#include "OS/UThread.h"
#include "OS/ThreadGroup.h"
#include "OS/StackTrace.h"
#include "Tracker.h"

using namespace os;

static int count1 = 0;
static int count2 = 0;

static void utFn1() {
	count1 = 1;
	UThread::leave();
	count1 = 10;
	UThread::leave();
	count1 = 4;
}

static void utFn2() {
	count2 = 1;
	UThread::leave();
	count2 = 10;
	UThread::leave();
	count2 = 4;
}

BEGIN_TEST(UThreadTest, OS) {
	count1 = 0;
	count2 = 0;

	UThread::spawn(util::simpleVoidFn(&utFn1));
	CHECK_EQ(count1, 0);
	CHECK_EQ(count2, 0);
	UThread::leave();
	UThread::spawn(util::simpleVoidFn(&utFn2));
	CHECK_EQ(count1, 1);
	CHECK_EQ(count2, 0);
	UThread::leave();
	CHECK_EQ(count1, 10);
	CHECK_EQ(count2, 1);
	UThread::leave();
	CHECK_EQ(count1, 4);
	CHECK_EQ(count2, 10);
	UThread::leave();
	CHECK_EQ(count1, 4);
	CHECK_EQ(count2, 4);
	CHECK(!UThread::any());
	UThread::leave();
	CHECK_EQ(count1, 4);
	CHECK_EQ(count2, 4);

} END_TEST

// Test functions.
static nat fnCallSum = 0;

static void natNatFn(nat a, nat b) {
	fnCallSum += a + b;
}

static void natFloatFn(nat a, float b) {
	fnCallSum += a + int(b);
}

static void trackerFn(Tracker a) {
	fnCallSum += a.data;
}

BEGIN_TEST(UThreadFnCallTest, OS) {
	fnCallSum = 0;

	{
		nat a = 10, b = 20;
		FnCall<void> call = fnCall().add(a).add(b);
		UThread::spawn(address(&natNatFn), false, call);
		UThread::leave();
		CHECK_EQ(fnCallSum, 30);
	}

	fnCallSum = 0;
	{
		nat a = 20;
		float b = 40.0f;
		FnCall<void> call = fnCall().add(a).add(b);
		UThread::spawn(address(&natFloatFn), false, call);
		UThread::leave();
		CHECK_EQ(fnCallSum, 60);
	}

	fnCallSum = 0;
	Tracker::clear();
	{
		Tracker t(12);
		FnCall<void> call = fnCall().add(t);
		UThread::spawn(address(&trackerFn), false, call);
		UThread::leave();
	}
	CHECK_EQ(fnCallSum, 12);
	CHECK(Tracker::clear());

} END_TEST


static void returnVoid(bool error) {
	if (error)
		throw UserError(L"ERROR");
}

static void voidParams(int a, int b) {
	assert(a == 10);
	assert(b == 20);
}

struct IntDummy {
	int data;

	IntDummy(int d) : data(d) {}

	int CODECALL large() {
		assert(data == 22);
		return data;
	}

	virtual int CODECALL virtualLarge() {
		assert(data == 22);
		return data;
	}

	void CODECALL voidMember(int a, int b) {
		assert((size_t)this == 10);
		assert(a == 20);
		assert(b == 30);
	}
};

static int returnInt(int v) {
	return v;
}

static int64 returnInt64(int64 v) {
	return v;
}

static float returnFloat(float v) {
	return v;
}

static double returnDouble(double v) {
	return v;
}

static Tracker returnTracker(int t) {
	if (t < 0)
		throw UserError(L"ERROR");
	return Tracker(t);
}

static int takeTracker(Tracker t) {
	if (t.data < 0)
		throw UserError(L"ERROR");
	return t.data;
}

struct DummyTracker {
	Tracker t;
	DummyTracker(int v) : t(v) {}

	Tracker CODECALL value() {
		return t;
	}
};

BEGIN_TEST(UThreadResultTest, OS) {
	{
		bool e = false;
		FnCall<void> params = fnCall().add(e);
		os::Future<void> r;
		UThread::spawn(address(returnVoid), false, params, r);
		CHECK_RUNS(r.result());

		e = true;
		os::Future<void> r2;
		UThread::spawn(address(returnVoid), false, params, r2);
		CHECK_ERROR(r2.result(), UserError);
	}

	{
		os::Future<void> r;
		int a = 10, b = 20;
		FnCall<void> p = fnCall().add(a).add(b);
		UThread::spawn(address(voidParams), false, p, r);
		CHECK_RUNS(r.result());
	}

	{
		os::Future<int> r;
		IntDummy d(22);
		IntDummy *pD = &d;
		FnCall<int> p = fnCall().add(pD);
		UThread::spawn(address(&IntDummy::large), true, p, r);
		CHECK_EQ(r.result(), 22);
	}

	{
		os::Future<int> r;
		IntDummy d(22);
		IntDummy *pD = &d;
		FnCall<int> p = fnCall().add(pD);
		UThread::spawn(address(&IntDummy::virtualLarge), true, p, r);
		CHECK_EQ(r.result(), 22);
	}

	{
		os::Future<void> r;
		void *a = (void *)10;
		int b = 20;
		int c = 30;
		FnCall<void> p = fnCall().add(a).add(b).add(c);
		UThread::spawn(address(&IntDummy::voidMember), true, p, r);
		CHECK_RUNS(r.result());
	}

	{
		int v = 10;
		FnCall<int> params = fnCall().add(v);
		os::Future<int> r;
		UThread::spawn(address(returnInt), false, params, r);
		CHECK_EQ(r.result(), 10);
	}

	{
		int64 v = 1024LL << 30LL;
		FnCall<int64> params = fnCall().add(v);
		os::Future<int64> r;
		UThread::spawn(address(returnInt64), false, params, r);
		CHECK_EQ(r.result() >> 30LL, 1024);
	}

	{
		float v = 13.37f;
		FnCall<float> params = fnCall().add(v);
		os::Future<float> r;
		UThread::spawn(address(returnFloat), false, params, r);
		CHECK_EQ(r.result(), 13.37f);
	}

	{
		double v = 13.37;
		FnCall<double> params = fnCall().add(v);
		os::Future<double> r;
		UThread::spawn(address(returnDouble), false, params, r);
		CHECK_EQ(r.result(), 13.37);
	}

	Tracker::clear();
	{
		int t = 22;
		FnCall<Tracker> params = fnCall().add(t);
		os::Future<Tracker> r;
		UThread::spawn(address(returnTracker), false, params, r);
		CHECK_EQ(r.result().data, 22);
		CHECK_EQ(r.result().data, 22);

		t = -2;
		os::Future<Tracker> r2;
		UThread::spawn(address(returnTracker), false, params, r2);
		CHECK_ERROR(r2.result(), UserError);
		CHECK_ERROR(r2.result(), UserError);
	}
	CHECK(Tracker::clear());

	Tracker::clear();
	{
		Tracker t(22);
		FnCall<int> params = fnCall().add(t);
		os::Future<int> r;
		UThread::spawn(address(takeTracker), false, params, r);
		CHECK_EQ(r.result(), 22);
		CHECK_EQ(r.result(), 22);

		t.data = -3;
		os::Future<int> r2;
		UThread::spawn(address(takeTracker), false, params, r2);
		CHECK_ERROR(r2.result(), UserError);
		CHECK_ERROR(r2.result(), UserError);
	}
	CHECK(Tracker::clear());

	Tracker::clear();
	{
		DummyTracker t(12);
		DummyTracker *pT = &t;
		FnCall<Tracker> params = fnCall().add(pT);
		os::Future<Tracker> r;
		UThread::spawn(address(&DummyTracker::value), true, params, r);
		CHECK_EQ(r.result().data, 12);
	}
	CHECK(Tracker::clear());

} END_TEST



static nat exceptions = 0;

static void exInner(nat depth = 0) {
	UThread::leave();
	if (depth == 3) {
		// PLN(format(stackTrace()));
		throw UserError(L"Testing!");
	} else {
		exInner(depth + 1);
	}
}

static void exRoot() {
	try {
		exInner();
	} catch (const UserError &e) {
		exceptions += e.what() == L"Testing!";
	}
}

BEGIN_TEST(UThreadExTest, OS) {
	exceptions = 0;

	UThread::spawn(util::simpleVoidFn(&exRoot));
	UThread::spawn(util::simpleVoidFn(&exRoot));

	// dumpStack();

	for (nat i = 0; i < 50; i++)
		UThread::leave();

	CHECK_EQ(exceptions, 2);
} END_TEST

static nat counter = 0;

static void recurseInner(nat depth = 0) {
	if (depth >= 3) // One extra since the last frame disappears sometimes...
		UThread::leave();
	else
		recurseInner(depth + 1);

	// Write global to avoid the compiler optimizing the call away.
	counter++;
}

static void recurseBase() {
	recurseInner(0);
}

// Count the number of frames that contain "recurseInner". Looks at an IP that is close to the start
// of 'recurseInner'. That way we don't have to rely on debug information for this to work.
static size_t countFn(::StackTrace &trace, const void *functionAddr) {
	const size_t MAX_DISTANCE = 256;
	size_t addr = (size_t)functionAddr;
	size_t count = 0;
	for (nat i = 0; i < trace.count(); i++) {
		size_t here = (size_t)trace[i].fnBase + trace[i].offset;
		if (addr <= here && here <= addr + MAX_DISTANCE)
			count++;
	}
	return count;
}

// Tests the detour function by doing stack traces on all UThreads.
// Does not verify that the stack traces are sensible, but ensures that we don't crash at least.
BEGIN_TEST(UThreadTracesTest, OS) {
	UThread::spawn(util::simpleVoidFn(&recurseBase));
	UThread::spawn(util::simpleVoidFn(&recurseBase));

	UThread::leave();

	vector<::StackTrace> traces = os::stackTraces(os::Thread::current());
	CHECK_EQ(traces.size(), 3);

	size_t numUThreads = 0;
	for (size_t i = 0; i < traces.size(); i++) {
		// PLN(L"Thread " << i);
		// PLN(format(traces[i]));
		if (countFn(traces[i], address(recurseInner)) >= 3)
			numUThreads++;
	}

	// Should find the two threads that are recursing now.
	// Note: If this test fails, it is possible that the debug information stopped working,
	// or that the compiler optimized the "recurseInner" recursion a bit too hard.
	CHECK_GTE(numUThreads, size_t(2));

	// Make threads exit.
	UThread::leave();
	UThread::leave();
} END_TEST


struct SemaTest {
	os::Sema sema;

	nat state;

	SemaTest() : sema(1), state(0) {}

	void run() {
		// Should not block.
		sema.down();

		state = 1;

		// Should block.
		sema.down();

		state = 2;
	}
};


BEGIN_TEST(UThreadSema, OS) {
	SemaTest t;
	UThread::spawn(util::memberVoidFn(&t, &SemaTest::run));

	CHECK_EQ(t.state, 0);
	UThread::leave();
	CHECK_EQ(t.state, 1);
	for (nat i = 0; i < 20; i++)
		UThread::leave();
	CHECK_EQ(t.state, 1);

	t.sema.up();
	UThread::leave();
	CHECK_EQ(t.state, 2);

} END_TEST

struct SemaInterop {
	os::Sema sema;

	nat state;

	SemaInterop() : sema(0), state(0) {}

	void run() {
		state = 1;

		// Should block.
		sema.down();

		state = 2;
	}

	void small() {
		state = 5;
	}

	void run2() {
		state = 3;
		sema.down();
		state = 4;
	}
};

BEGIN_TEST(UThreadSemaInterop, OS) {
	SemaInterop t;
	ThreadGroup group;

	// Make the main thread wait for a UThread that is not in its running state.
	{
		os::Thread on = os::Thread::spawn(util::memberVoidFn(&t, &SemaInterop::run), group);
		Sleep(30);
		CHECK_EQ(t.state, 1);

		// Another thread, should block.
		UThread::spawn(util::memberVoidFn(&t, &SemaInterop::run2), &on);
		Sleep(30);
		CHECK_EQ(t.state, 3);
	}

	// Now, release the original thread and make sure it does not exit until the 'run2' call is complete.
	t.sema.up();
	Sleep(30);
	CHECK_EQ(t.state, 2);

	t.sema.up();
	Sleep(30);
	CHECK_EQ(t.state, 4);

	// No threads to schedule when a sema should block. This should make
	// the other thread spin in UThread::wait() for a while.
	os::Thread::spawn(util::memberVoidFn(&t, &SemaInterop::run), group);
	Sleep(30);
	CHECK_EQ(t.state, 1);
	t.sema.up();
	Sleep(30);
	CHECK_EQ(t.state, 2);

	// No threads to schedule when a sema would block while starting another UThread.
	{
		os::Thread on = os::Thread::spawn(util::memberVoidFn(&t, &SemaInterop::run), group);
		Sleep(30);
		CHECK_EQ(t.state, 1);

		// Launch another UThread!
		UThread::spawn(util::memberVoidFn(&t, &SemaInterop::small), &on);
		Sleep(30);
		CHECK_EQ(t.state, 5);

		t.sema.up();
	}
	Sleep(30);
	CHECK_EQ(t.state, 2);

} END_TEST
