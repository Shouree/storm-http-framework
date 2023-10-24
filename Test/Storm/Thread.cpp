#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Debug.h"
#include "Compiler/Exception.h"

BEGIN_TEST(BSThread, BS) {
	using namespace storm::debug;

	Engine &e = gEngine();

	Thread *thread = runFn<Thread *>(S("tests.bs.getThread"));
	CHECK_NEQ(thread, storm::Compiler::thread(e));
	thread = runFn<Thread *>(S("tests.bs.getCompilerThread"));
	CHECK_EQ(thread, storm::Compiler::thread(e));

	CHECK_EQ(runFn<Int>(S("tests.bs.postInt")), 9);

	// Check so that we keep reference counting correct.
	DbgVal::clear();
	CHECK_EQ(runFn<Int>(S("tests.bs.postDbgVal")), 18);
	CHECK(DbgVal::clear());

	// Basic thread tests.
	CHECK_EQ(runFn<Int>(S("tests.bs.postObject")), 13);
	CHECK_EQ(runFn<Int>(S("tests.bs.postVal")), 33);
	CHECK_EQ(runFn<Int>(S("tests.bs.postMaybeVal")), 33);
	CHECK_EQ(runFn<Int>(S("tests.bs.threadObj")), 20);
	CHECK_EQ(runFn<Int>(S("tests.bs.threadActor")), 20);
	CHECK_EQ(runFn<Int>(S("tests.bs.actorObj")), 31);
	CHECK_EQ(runFn<Int>(S("tests.bs.actorDerObj")), 22);

	// Future tests.
	CHECK_EQ(runFn<Int>(S("tests.bs.basicFuture")), 8);
	CHECK_EQ(runFn<Int>(S("tests.bs.valueFuture")), 8);
	CHECK_EQ(runFn<Int>(S("tests.bs.intFuture")), 22);
	CHECK_RUNS(runFn<void>(S("tests.bs.noResultFuture")));

	// Check 'spawn'.
	CHECK_EQ(runFn<Int>(S("tests.bs.asyncPostInt")), 9);
	CHECK_EQ(runFn<Int>(S("tests.bs.asyncPostObject")), 13);
	CHECK_EQ(runFn<Int>(S("tests.bs.asyncPostObject2")), 13);
	CHECK_EQ(runFn<Int>(S("tests.bs.asyncPostVal")), 33);
	CHECK_RUNS(runFn<void>(S("tests.bs.spawnVoid")));

	// Check 'spawn' to the same thread. This should *not* cause any deep copies (we don't
	// differentiate between classes and values here - we know that works properly from earlier tests).
	CHECK_EQ(runFn<Int>(S("tests.bs.asyncPostSame")), 4);
	CHECK_EQ(runFn<Int>(S("tests.bs.asyncReturnSame")), 4);
	CHECK_EQ(runFn<Int>(S("tests.bs.asyncPostSameThread")), 4);

	// Check variable accesses in other threads.
	CHECK_EQ(runFn<Int>(S("tests.bs.threadVarAccess")), 6); // 1 copy, 1 deep copy. Starts at 4.
	CHECK_ERROR(runFn<void>(S("tests.bs.threadVarAssign")), SyntaxError);

	// Check so that we don't mistake variables named "this" for the real thing.
	CHECK_EQ(runFn<Int>(S("tests.bs.defaultThis")), 4); // Standard behavior, no copies.
	CHECK_EQ(runFn<Int>(S("tests.bs.customThis")), 6); // 1 copy + 1 deep copy. Starts at 4.

	// Check the semantics of the "spawn(...)" syntax:
	CHECK_EQ(runFn<Int>(S("tests.bs.asyncPostExplicit")), 6); // 1 copy + 1 deep copy, starts at 4.
	CHECK_ERROR(runFn<void>(S("tests.bs.asyncPostExplictError")), SyntaxError);
} END_TEST
