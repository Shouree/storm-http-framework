#include "stdafx.h"
#include "Fn.h"
#include "Core/Fn.h"
#include "Compiler/Debug.h"

using namespace storm::debug;

static Int addTwo(Int v) {
	return v + 2;
}

static Int returnFive() {
	return 5;
}

static DbgVal returnDbgVal(Int v) {
	return DbgVal(v);
}

static Dbg *returnDbg(Int v) {
	return new (gEngine()) Dbg(v);
}

static Int fromDbg(Dbg *dbg) {
	return dbg->get();
}

static Str *echoStr(Str *v) {
	return v;
}

static Bool bool1() {
	return true;
}

static Bool bool2() {
	return false;
}

static void consumeStr(Str *v) {}

static Fn<DbgVal, Dbg *> *dbgFn(int id) {
	typedef Fn<DbgVal, Dbg *> FnPtr;
	return runFn<FnPtr *>(S("tests.bs.createDbgFn"), id);
}

static Fn<DbgVal> *voidFn(int id) {
	typedef Fn<DbgVal> FnPtr;
	return runFn<FnPtr *>(S("tests.bs.createVoidFn"), id);
}

static Fn<DbgVal, DbgActor *> *actorFn(int id) {
	typedef Fn<DbgVal, DbgActor *> FnPtr;
	return runFn<FnPtr *>(S("tests.bs.createActorFn"), id);
}

BEGIN_TEST(FnPtrTest, BS) {
	Engine &e = gEngine();

	Fn<Int, Int> *fn = fnPtr(e, &addTwo);
	CHECK_EQ(fn->call(10), 12);

	Fn<Int> *voidFn = fnPtr(e, &returnFive);
	CHECK_EQ(voidFn->call(), 5);

	DbgActor * actor = new (e) DbgActor();
	Fn<Str *, Str *> *strFn = fnPtr(e, &DbgActor::echo, actor);

	DbgVal::clear();
	Fn<DbgVal, Int> *dbgValFn = fnPtr(e, &returnDbgVal);
	CHECK_EQ(dbgValFn->call(23), DbgVal(23));
	CHECK(DbgVal::clear());

	Fn<Dbg *, Int> *dbgFn = fnPtr(e, &returnDbg);
	CHECK_EQ(dbgFn->call(20)->get(), 20);

	Fn<Str *, Str *> *echoStr = fnPtr(e, &::echoStr);
	Fn<void, Str *> *consumeStr = fnPtr(e, &::consumeStr);

	Fn<Int, Dbg *> *consumeDbg = fnPtr(e, &::fromDbg);
	CHECK_EQ(consumeDbg->call(new (e) Dbg(2)), 2);

	Fn<Bool> *b1 = fnPtr(e, &::bool1);
	Fn<Bool> *b2 = fnPtr(e, &::bool2);
	CHECK_EQ(b1->call(), true);
	CHECK_EQ(b2->call(), false);

	// Run some code in Storm!
	CHECK_RUNS(runFn<void>(S("tests.bs.consumeStr"), consumeStr));
	CHECK_EQ(runFn<Int>(S("tests.bs.consumeDbg"), consumeDbg), 23);
	CHECK_EQ(runFn<Int>(S("tests.bs.runFnPtr"), fn), 12);
	CHECK_EQ(runFn<Int>(S("tests.bs.runFnPtr"), voidFn), 5);
	CHECK_EQ(runFn<Int>(S("tests.bs.runStrFnPtr"), strFn), 22);
	CHECK_EQ(runFn<Int>(S("tests.bs.runStrFnPtr"), echoStr), 22);
	DbgVal::clear();
	CHECK_EQ(runFn<Int>(S("tests.bs.runFnPtr"), dbgValFn), 23);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("tests.bs.runFnPtr"), dbgFn), 21);
	CHECK_EQ(runFn<Int>(S("tests.bs.runBoolFn"), b1), 1);
	CHECK_EQ(runFn<Int>(S("tests.bs.runBoolFn"), b2), 0);

	b1 = runFn<Fn<Bool> *>(S("tests.bs.createBoolFn"), true);
	b2 = runFn<Fn<Bool> *>(S("tests.bs.createBoolFn"), false);
	CHECK_EQ(b1->call(), true);
	CHECK_EQ(b2->call(), false);
	CHECK_EQ(runFn<Int>(S("tests.bs.runBoolFn"), b1), 1);
	CHECK_EQ(runFn<Int>(S("tests.bs.runBoolFn"), b2), 0);

} END_TEST


BEGIN_TEST(StormFnPtrTest, BS) {
	Engine &e = gEngine();

	// Return a function pointer from Storm!
	{
		typedef Fn<Int, Int> IIFn;
		IIFn *v = runFn<IIFn *>(S("tests.bs.createIntFn"));
		CHECK_EQ(v->call(10), 20);
	}


	{
		Dbg *dbg = new (e) Dbg(5);
		CHECK_EQ(dbgFn(0)->call(dbg), DbgVal(15));
		CHECK_EQ(dbgFn(1)->call(dbg), DbgVal(25));
	}

	{
		CHECK_EQ(voidFn(0)->call(), DbgVal(20));
		CHECK_EQ(voidFn(1)->call(), DbgVal(12));
		CHECK_EQ(voidFn(2)->call(), DbgVal(22));
	}

	// Actors.
	{
		DbgActor *actor = new (e) DbgActor(7);
		CHECK_EQ(actorFn(0)->call(actor), DbgVal(27));
		CHECK_EQ(actorFn(1)->call(actor), DbgVal(11));
	}

} END_TEST

BEGIN_TEST(StormFnPtrCopyTest, BS) {
	// Previously, the copy-ctor of Fn<T> was broken. Surfaced in sort() for example.
	CHECK_EQ(runFn<Int>(S("tests.bs.copyIntPredicate")), 6);
} END_TEST

// Run the function pointer both through Storm and C++ and compare the results.
static int runPtr(Fn<Int, Dbg *> *ptr, int start) {
	Dbg *dbg = runFn<Dbg *>(S("tests.bs.dbgTrack"), start);
	int storm = runFn<Int>(S("tests.bs.runPtr"), ptr, dbg);
	int cpp = ptr->call(dbg);
	if (storm != cpp) {
		PLN("Storm and C++ gives different results!");
		return -1;
	}
	return storm;
}

// Run the function pointer through another thread (only Storm) and see the result.
static int runPtrOther(Fn<Int, Dbg *> *ptr, int start) {
	Dbg *dbg = runFn<Dbg *>(S("tests.bs.dbgTrack"), start);
	return runFn<Int>(S("tests.bs.runPtrOther"), ptr, dbg);
}


static Fn<Int, Dbg *> *dbgPtr(int id) {
	return runFn<Fn<Int, Dbg *> *>(S("tests.bs.trackParam"), id);
}

BEGIN_TEST(FnPtrThreadTest, BS) {
	CHECK_EQ(runPtr(dbgPtr(0), 1), 1);
	CHECK_EQ(runPtr(dbgPtr(1), 1), 10101);
	CHECK_EQ(runPtr(dbgPtr(2), 1), 1);
	CHECK_EQ(runPtr(dbgPtr(3), 1), 10101);
	CHECK_EQ(runPtr(dbgPtr(4), 1), 1);

	// Note: these are always copied once because we're switching to another thread before calling the
	// function pointer.
	CHECK_EQ(runPtrOther(dbgPtr(0), 1), 10101);
	CHECK_EQ(runPtrOther(dbgPtr(1), 1), 10101);
	CHECK_EQ(runPtrOther(dbgPtr(2), 1), 20201);
	CHECK_EQ(runPtrOther(dbgPtr(3), 1), 10101);
	CHECK_EQ(runPtrOther(dbgPtr(4), 1), 20201);

	// Fn<Int, Dbg *> *fromDbg = fnPtr(e, &::fromDbg);
	CHECK_EQ(runPtrOther(dbgPtr(0), 1), 10101);

	// Actor as the first parameter...
	{
		Dbg *dbg = runFn<Dbg *>(S("tests.bs.dbgTrack"), 1);
		TObject *dbgActor = runFn<TObject *>(S("tests.bs.createInActor"));
		Fn<Int, TObject *, Dbg *> *fn =
			runFnUnsafe<Fn<Int, TObject *, Dbg *> *>(S("tests.bs.trackActor"));
		int storm = runFn<Int>(S("tests.bs.runPtrActor"), fn, dbg);
		int cpp = fn->call(dbgActor, dbg);
		CHECK_EQ(storm, 10101);
		CHECK_EQ(cpp, 10101);
	}

	// Return values.
	{
		Fn<Dbg *> *fn = runFn<Fn<Dbg *> *>(S("tests.bs.dbgFnPtr"));

		Dbg *cpp = fn->call();
		CHECK_EQ(cpp->get(), 10101);

		int storm = runFn<Int>(S("tests.bs.callDbgFnPtr"), fn);
		CHECK_EQ(storm, 10101);
	}

	// Automatic parameter type deduction.
	{
		CHECK_EQ(runFn<Int>(S("tests.bs.autoFn")), 20);
		CHECK_EQ(runFn<Int>(S("tests.bs.autoMemberFn")), 15);
	}

} END_TEST
