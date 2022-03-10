#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Debug.h"
#include "Compiler/Exception.h"
#include "Compiler/Package.h"
#include "Core/Timing.h"
#include "Core/Set.h"
#include "Core/Variant.h"

using storm::debug::DbgVal;
using storm::debug::Dbg;
using storm::debug::DbgActor;

BEGIN_TEST(Priority, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.prio1")), 203);
	CHECK_EQ(runFn<Int>(S("tests.bs.prio2")), 203);
	CHECK_EQ(runFn<Int>(S("tests.bs.prio3")), 230);
	CHECK_EQ(runFn<Int>(S("tests.bs.prio4")), 230);
	CHECK_EQ(runFn<Int>(S("tests.bs.prio5")), 6010);
	CHECK_EQ(runFn<Int>(S("tests.bs.prio6")), 11);
	CHECK_EQ(runFn<Int>(S("tests.bs.prio7")), 0);
	CHECK_EQ(runFn<Int>(S("tests.bs.prio8")), 2);
	CHECK_EQ(runFn<Int>(S("tests.bs.prio9")), 2);

	CHECK_EQ(runFn<Int>(S("tests.bs.combOp1")), 24);
	CHECK_EQ(runFn<Int>(S("tests.bs.combOp2")), 24);
} END_TEST

BEGIN_TEST(Overload, BS) {
	// If this fails, the compiler does not choose the correct overload in the case of ambiguities.
	CHECK_EQ(runFn<Int>(S("tests.bs.testOverload"), 1), 1);
	CHECK_EQ(runFn<Int>(S("tests.bs.testOverload"), 0), 2);
} END_TEST

BEGIN_TEST(InheritanceTest, BS) {
	using storm::debug::Dbg;

	// Some inheritance testing.
	CHECK_EQ(runFn<Int>(S("tests.bs.testBase")), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.testDerived")), 20);

	// Using C++ classes as base.
	CHECK_EQ(runFn<Int>(S("tests.bs.testCpp"), 1), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.testCpp"), 2), 20);

	Dbg *created = runFn<Dbg *>(S("tests.bs.createCpp"), 1);
	CHECK_EQ(created->get(), 10);
	created = runFn<Dbg *>(S("tests.bs.createCpp"), 2);
	CHECK_EQ(created->get(), 20);

	// Use of 'super'.
	CHECK_EQ(runFn<Int>(S("tests.bs.testSuperExpr")), 30);

	// Use of 'as'.
	CHECK_EQ(runFn<Int>(S("tests.bs.testAsExpr"), 0), 30);
	CHECK_EQ(runFn<Int>(S("tests.bs.testAsExpr"), 1), 12);
	CHECK_EQ(runFn<Int>(S("tests.bs.testAsExpr2"), 0), 30);
	CHECK_EQ(runFn<Int>(S("tests.bs.testAsExpr2"), 1), 12);

	// Inheritance together with visibility. Private members should not interact with other classes.
	CHECK_EQ(runFn<Int>(S("tests.bs.testAccess"), 0, 0), 10); // Base, private
	CHECK_EQ(runFn<Int>(S("tests.bs.testAccess"), 1, 0), 10); // Derived, private
	CHECK_EQ(runFn<Int>(S("tests.bs.testAccess"), 0, 1), 10); // Base, protected
	CHECK_EQ(runFn<Int>(S("tests.bs.testAccess"), 1, 1), 20); // Derived, protected

	// These should fail for various reasons.
	CHECK_ERROR(runFn<Int>(S("tests.bs.testCallProt")), SyntaxError);
	CHECK_ERROR(runFn<Int>(S("tests.bs.testCallPriv")), SyntaxError);
	CHECK_ERROR(runFn<Int>(S("tests.bs.testPrivSuper")), SyntaxError);

	// Inner classes should be able to access private things.
	CHECK_EQ(runFn<Int>(S("tests.bs.testInner")), 25);
} END_TEST

BEGIN_TEST(AbstractTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.createNoAbstract")), 10);
	CHECK_ERROR(runFn<Int>(S("tests.bs.createAbstract")), InstantiationError);

	CHECK_EQ(runFn<Int>(S("tests.bs.createCppNoAbstract")), 110);
	CHECK_ERROR(runFn<Int>(S("tests.bs.createCppAbstract")), InstantiationError);

	CHECK_ERROR(runFn<Int>(S("tests.bs.cppCallSuper")), AbstractFnCalled);
	CHECK_ERROR(runFn<Int>(S("tests.bs.cppAbstractSuper")), AbstractFnCalled);

	debug::DbgAbstractDtor::destroyCount = 0;
	CHECK_RUNS(runFn<Int>(S("tests.bs.cppAbstractDtor")));
	for (int i = 0; i < 10; i++) {
		gEngine().gc.collect();
		if (debug::DbgAbstractDtor::destroyCount)
			break;
	}
	CHECK_NEQ(debug::DbgAbstractDtor::destroyCount, 0);
} END_TEST

BEGIN_TEST(FinalTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.createFinalBase")), 10);
	// Overriding a final function.
	CHECK_ERROR(runFn<Int>(S("tests.bs.createFinalA")), TypedefError);
	// Using 'override' properly.
	CHECK_EQ(runFn<Int>(S("tests.bs.createFinalB")), 30);
	// Not overriding, even though 'override' is specified.
	CHECK_ERROR(runFn<Int>(S("tests.bs.createFinalC")), TypedefError);
	// Overrides, but with invalid return type.
	CHECK_ERROR(runFn<Int>(S("tests.bs.createFinalD")), TypedefError);
} END_TEST

BEGIN_TEST(StaticTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.testStatic")), 52);
	CHECK_EQ(runFn<Int>(S("tests.bs.testStaticInheritance"), 0), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.testStaticInheritance"), 1), 10);
} END_TEST

BEGIN_TEST(VisibilityTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.localPrivate")), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.localInner")), 10);
	CHECK_ERROR(runFn<Int>(S("tests.bs.localSecret")), SyntaxError);

	CHECK_ERROR(runFn<Int>(S("tests.bs.remotePrivate")), SyntaxError);
	CHECK_ERROR(runFn<Int>(S("tests.bs.remoteInner")), SyntaxError);
	CHECK_ERROR(runFn<Int>(S("tests.bs.remoteSecret")), SyntaxError);
} END_TEST

/**
 * Values.
 */

BEGIN_TEST(ValueTest, BS) {
	// Values.
	DbgVal::clear();
	CHECK_EQ(runFn<Int>(S("tests.bs.testValue")), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("tests.bs.testDefInit")), 10);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("tests.bs.testValAssign")), 20);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("tests.bs.testValCopy")), 20);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("tests.bs.testValCtor")), 7);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("tests.bs.testValParam")), 16);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<Int>(S("tests.bs.testValReturn")), 22);
	CHECK(DbgVal::clear());
	CHECK_EQ(runFn<DbgVal>(S("tests.bs.createVal"), 20), DbgVal(20));
	CHECK(DbgVal::clear());
	CHECK_EQ((runFn<Int, DbgVal>(S("tests.bs.asVal"), DbgVal(11))), 13);
	CHECK(DbgVal::clear());
	// TODO: See if we can test so that destructors are executed from within classes/actors.
} END_TEST

BEGIN_TEST(CustomValueTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.testCustomValue")), -300);
	CHECK_EQ(runFn<Int>(S("tests.bs.testRefVal"), 24), 24);
	CHECK_EQ(runFn<Int>(S("tests.bs.testCopyRefVal"), 24), 24);
	CHECK_EQ(runFn<Int>(S("tests.bs.testAssignRefVal"), 24), 24);
	CHECK_EQ(runFn<Int>(S("tests.bs.testValVal"), 22), 22);
	CHECK_EQ(runFn<Int>(S("tests.bs.testCopyValVal"), 22), 22);
	CHECK_EQ(runFn<Int>(S("tests.bs.testAssignValVal"), 22), 22);
} END_TEST

BEGIN_TEST(ValueMemberTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.testVirtualVal1")), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.testVirtualVal2")), 20);
	CHECK_EQ(runFn<Int>(S("tests.bs.testVirtualVal3")), 15);
	// This test is really good in release builts. For VS2008, the compiler uses
	// the return value (in eax) of v->asDbgVal(), and crashes if we fail to return
	// a correct value. In debug builds, the compiler usually re-loads the value
	// itself instead.
	Dbg *v = runFn<Dbg *>(S("tests.bs.testVirtualVal4"));
	CHECK_EQ(v->asDbgVal().v, 20);

	// Does the thread thunks correctly account for the special handling of member functions?
	CHECK_EQ(runFn<Int>(S("tests.bs.testActorVal")), 10);
} END_TEST


/**
 * Autocast.
 */

BEGIN_TEST(AutocastTest, BS) {
	// Check auto-casting from int to nat.
	CHECK_EQ(runFn<Int>(S("tests.bs.castToNat")), 20);
	CHECK_EQ(runFn<Int>(S("tests.bs.castToMaybe")), 20);
	CHECK_EQ(runFn<Int>(S("tests.bs.downcastMaybe")), 20);
	CHECK_RUNS(runFn<void>(S("tests.bs.ifCast")));
	CHECK_EQ(runFn<Int>(S("tests.bs.autoCast"), 5), 10);
	CHECK_EQ(runFn<Float>(S("tests.bs.promoteCtor")), 2);
	CHECK_EQ(runFn<Float>(S("tests.bs.promoteInit")), 8);
	CHECK_EQ(runFn<Nat>(S("tests.bs.initNat")), 20);
	CHECK_EQ(runFn<Float>(S("tests.bs.opFloat")), 5.0f);
} END_TEST


/**
 * Automatic generation of operators.
 */

BEGIN_TEST(OperatorTest, BS) {
	CHECK_EQ(runFn<Bool>(S("tests.bs.opLT"), 10, 20), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opLT"), 20, 10), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opLT"), 10, 10), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opGT"), 10, 20), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opGT"), 20, 10), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opGT"), 10, 10), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opLTE"), 10, 20), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opLTE"), 20, 10), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opLTE"), 20, 20), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opGTE"), 10, 20), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opGTE"), 20, 10), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opGTE"), 10, 10), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opEQ"), 10, 20), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opEQ"), 20, 10), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opEQ"), 10, 10), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opNEQ"), 10, 20), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opNEQ"), 20, 10), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opNEQ"), 10, 10), false);

	// Check the == operator for actors.
	DbgActor *a = new (gEngine()) DbgActor(1);
	DbgActor *b = new (gEngine()) DbgActor(2);

	CHECK_EQ(runFn<Bool>(S("tests.bs.opActorEQ"), a, b), false);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opActorEQ"), a, a), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opActorNEQ"), a, b), true);
	CHECK_EQ(runFn<Bool>(S("tests.bs.opActorNEQ"), a, a), false);
} END_TEST

BEGIN_TEST(SetterTest, BS) {
	// The hard part with this test is not getting the correct result, it is getting it to compile!
	CHECK_EQ(runFn<Int>(S("tests.bs.testSetter")), 20);
	CHECK_EQ(runFn<Int>(S("tests.bs.testSetterPair")), 20);
	CHECK_EQ(runFn<Float>(S("tests.bs.testSetterCpp")), 50.0f);

	// When trying to use anything else as a setter, a syntax error should be thrown!
	CHECK_ERROR(runFn<Int>(S("tests.bs.testNoSetter")), SyntaxError);
} END_TEST


/**
 * Type system.
 */

BEGIN_TEST(TypesTest, BS) {
	CHECK_ERROR(runFn<void>(S("tests.bs.invalidDecl")), SyntaxError);
	CHECK_ERROR(runFn<void>(S("tests.bs.voidExpr")), SyntaxError);
} END_TEST


/**
 * Maybe.
 */

BEGIN_TEST(MaybeTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybe"), 0), 0);
	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybe"), 1), 2);
	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybe"), 2), 6);

	CHECK_EQ(runFn<Int>(S("tests.bs.assignMaybe")), 1);
	CHECK_ERROR(runFn<void>(S("tests.bs.assignError")), SyntaxError);

	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.maybeToS"), 0)), L"null");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.maybeToS"), 1)), L"ok");

	CHECK_EQ(runFn<Int>(S("tests.bs.maybeInheritance")), 10);

	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeInv"), 0), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeInv"), 1), 1);

	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeInv2"), 0), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeInv2"), 1), 1);

	CHECK_EQ(runFn<Int>(S("tests.bs.testPlainUnless"), 5), 5);
	CHECK_EQ(runFn<Int>(S("tests.bs.testPlainUnless"), 10), 18);

	// Values!
	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeValue"), 0), 8);
	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeValue"), 1), 1);

	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeValueAny"), 0), 0);
	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeValueAny"), 1), 1);

	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.testMaybeValueToS"), 0)), L"null");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.testMaybeValueToS"), 5)), L"5");

	CHECK_EQ(runFn<Int>(S("tests.bs.testMaybeNullInit")), -1);

	CHECK_EQ(runFn<Int>(S("tests.bs.testExplicitSelf")), 2);
	CHECK_EQ(runFn<Int>(S("tests.bs.testExplicitOther")), 2);

	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.testReceiveMaybe"))), L"Iterator: ab|>c");

	CHECK_EQ(runFn<Int>(S("tests.bs.threadMaybeClass")), 6);
	CHECK_EQ(runFn<Int>(S("tests.bs.threadMaybeValue")), 6);
} END_TEST


/**
 * Constructors.
 */

BEGIN_TEST(StormCtorTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.ctorTest")), 50);
	CHECK_EQ(runFn<Int>(S("tests.bs.ctorTest"), 10), 30);
	CHECK_EQ(runFn<Int>(S("tests.bs.ctorTestDbg"), 10), 30);
	CHECK_RUNS(runFn<void>(S("tests.bs.ignoreCtor")));
	CHECK_EQ(runFn<Int>(S("tests.bs.ctorDerTest"), 2), 6);
	CHECK_ERROR(runFn<Int>(S("tests.bs.ctorErrorTest")), CodeError);
	CHECK_ERROR(runFn<Int>(S("tests.bs.memberAssignErrorTest")), CodeError);
	CHECK_EQ(runFn<Int>(S("tests.bs.testDefaultCtor")), 60);
	CHECK_EQ(runFn<Int>(S("tests.bs.testImplicitInit")), 50);

	// Initialization order.
	CHECK_EQ(runFn<Int>(S("tests.bs.checkInitOrder")), 321);
	CHECK_EQ(runFn<Int>(S("tests.bs.checkInitOrder2")), 123);

	// Two-step initialization.
	CHECK_RUNS(runFn<void>(S("tests.bs.twoStepInit")));
	CHECK_ERROR(runFn<void>(S("tests.bs.twoStepFail")), SyntaxError);
} END_TEST


/**
 * Scoping.
 */

BEGIN_TEST(ScopeTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.testScopeCls")), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.testClassMember")), 20);
	CHECK_EQ(runFn<Int>(S("tests.bs.testClassNonmember")), 20);
} END_TEST


/**
 * Units.
 */

BEGIN_TEST(UnitTest, BS) {
	CHECK_EQ(runFn<Duration>(S("tests.bs.testUnit")), time::ms(1) + time::s(1));
} END_TEST


/**
 * Return.
 */

BEGIN_TEST(ReturnTest, BS) {
	// Return integers.
	CHECK_EQ(runFn<Int>(S("tests.bs.returnInt"), 10), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.returnInt"), 40), 20);

	// Return strings.
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.returnStr"), 10)), L"no");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.returnStr"), 40)), L"40");

	// Return values.
	DbgVal::clear();
	CHECK_EQ(runFn<DbgVal>(S("tests.bs.returnDbgVal"), 11).get(), 10);
	CHECK_EQ(runFn<DbgVal>(S("tests.bs.returnDbgVal"), 30).get(), 20);
	CHECK(DbgVal::clear());

	// Return type checking, interaction with 'no-return' returns.
	CHECK_EQ(runFn<Int>(S("tests.bs.returnAlways"), 22), 22);
	CHECK_EQ(runFn<Int>(S("tests.bs.deduceType"), 21), 22);
	CHECK_EQ(runFn<Int>(S("tests.bs.prematureReturn"), 20), 30);
} END_TEST

/**
 * Break/continue.
 */
BEGIN_TEST(BreakTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.breakLoop"), 5), 6);
	CHECK_EQ(runFn<Int>(S("tests.bs.breakWithCond"), 0), 1);
	CHECK_EQ(runFn<Int>(S("tests.bs.breakWithCond"), 1), 2);
	CHECK_EQ(runFn<Int>(S("tests.bs.breakWithCond"), 2), 3);
	CHECK_EQ(runFn<Int>(S("tests.bs.breakFor"), 4), 6);
	CHECK_EQ(runFn<Int>(S("tests.bs.breakFor"), 10), 21);

	Array<Int> *data = new (gEngine()) Array<Int>();
	*data << 1 << 10 << 2 << 10;
	CHECK_EQ(runFn<Int>(S("tests.bs.breakForeach"), data), 1);
} END_TEST

BEGIN_TEST(ContinueTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.continueLoop"), 5), 9);
	CHECK_EQ(runFn<Int>(S("tests.bs.continueFor"), 5), 4);

	Array<Int> *data = new (gEngine()) Array<Int>();
	*data << 1 << 10 << 2 << 10;
	CHECK_EQ(runFn<Int>(S("tests.bs.continueForeach"), data), 3);
} END_TEST


/**
 * Arrays.
 */

BEGIN_TEST(BSArrayTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.testArray")), 230);
	CHECK_EQ(runFn<Int>(S("tests.bs.testIntArray")), 95);
	CHECK_EQ(runFn<Int>(S("tests.bs.testInitArray")), 1337);
	CHECK_EQ(runFn<Int>(S("tests.bs.testInitAutoArray")), 1234);
	CHECK_EQ(runFn<Int>(S("tests.bs.testAutoArray")), 0);
	CHECK_EQ(runFn<Int>(S("tests.bs.testCastArray")), 2);
	CHECK_EQ(runFn<Int>(S("tests.bs.testIterator")), 15);
	CHECK_EQ(runFn<Int>(S("tests.bs.testIteratorIndex")), 16);

	// Interoperability.
	Array<Int> *r = runFn<Array<Int> *>(S("tests.bs.createValArray"));
	CHECK_EQ(r->count(), 20);
	for (nat i = 0; i < 20; i++) {
		CHECK_EQ(r->at(i), i);
	}

	// Sorting.
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.sortArray"))), L"[1, 2, 3, 4, 5]");
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.sortedArray"))), L"[1, 2, 3, 4, 5][5, 4, 3, 2, 1]");
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.sortArrayP"))), L"[3, 4, 5, 1, 2]");
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.sortedArrayP"))), L"[3, 4, 5, 1, 2][5, 4, 3, 2, 1]");
} END_TEST

BEGIN_TEST(BSPQTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.pqSecond")), 8);
	CHECK_EQ(runFn<Int>(S("tests.bs.pqInit")), 8);
	CHECK_EQ(runFn<Int>(S("tests.bs.pqCompare")), 5);
	CHECK_EQ(runFn<Int>(S("tests.bs.pqCompareInit")), 2);
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.pqStr"))), L"World");
} END_TEST

BEGIN_TEST(BSQueueTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.queueInt")), 10);
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.queueString"))), L"World");
} END_TEST

/**
 * Map.
 */

BEGIN_TEST(BSMapTest, BS) {
	Engine &e = gEngine();
	{
		Array<Int> *keys = new (e) Array<Int>();
		Array<Int> *vals = new (e) Array<Int>();

		keys->push(10);
		keys->push(100);
		vals->push(5);
		vals->push(8);

		Map<Int, Int> *map = runFn<Map<Int, Int> *>(S("tests.bs.intMapTest"), keys, vals);
		CHECK_EQ(map->count(), 2);
		CHECK_EQ(map->get(10), 5);
		CHECK_EQ(map->get(100), 8);

		CHECK_EQ(runFn<Int>(S("tests.bs.iterateMap"), map), 850);
	}

	{
		Array<Str *> *keys = new (e) Array<Str *>();
		Array<Str *> *vals = new (e) Array<Str *>();

		keys->push(new (e) Str(S("A")));
		keys->push(new (e) Str(S("B")));
		vals->push(new (e) Str(S("80")));
		vals->push(new (e) Str(S("90")));

		Map<Str *, Str *> *map = runFn<Map<Str *, Str *> *>(S("tests.bs.strMapTest"), keys, vals);
		CHECK_EQ(map->count(), 2);
		CHECK_EQ(toS(runFn<Str *>(S("tests.bs.readStrMap"), map, new (e) Str(S("A")))), L"80");
		CHECK_EQ(toS(runFn<Str *>(S("tests.bs.readStrMap"), map, new (e) Str(S("B")))), L"90");
	}

	{
		CHECK_EQ(runFn<Int>(S("tests.bs.defaultMapInt")), 0);
		CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.defaultMapStr"))), L"");
		Array<Str *> *arr = runFn<Array<Str *> *>(S("tests.bs.defaultMapArray"));
		CHECK_EQ(arr->count(), 0);

		Map<Int, Str *> *map = new (e) Map<Int, Str *>();
		CHECK_EQ(::toS(map->at(1)), L"");
	}

} END_TEST


/**
 * Set.
 */

BEGIN_TEST(BSSetTest, BS) {
	Set<Int> *s = new (gEngine()) Set<Int>();
	s->put(100);
	s->put(80);
	s->put(9);

	CHECK_EQ(runFn<Int>(S("tests.bs.iterateSet"), s), 189);
} END_TEST

BEGIN_TEST(BSWeakSetTest, BS) {
	// Note: "a" has to be volatile, otherwise the compiler sometimes optimizes the variable away
	// when we need it to keep values in the weak set alive!
	Array<DbgActor *> *volatile data = new (gEngine()) Array<DbgActor *>();
	data->push(new (gEngine()) DbgActor(10));
	data->push(new (gEngine()) DbgActor(80));
	data->push(new (gEngine()) DbgActor(200));

	WeakSet<DbgActor> *s = new (gEngine()) WeakSet<DbgActor>();
	for (Nat i = 0; i < data->count(); i++)
		s->put(data->at(i));

	CHECK_EQ(runFn<Int>(S("tests.bs.iterateWeakSetPlain"), s), 290);
	CHECK_EQ(runFn<Int>(S("tests.bs.iterateWeakSet"), s), 290);

	// To make sure that the compiler does not optimize the data array away...
	Nat tmp = 0;
	for (Nat i = 0; i < data->count(); i++)
		tmp += data->at(i)->get();
	CHECK_EQ(tmp, 290);
} END_TEST


/**
 * Enums.
 */

BEGIN_TEST(BSEnumTest, BS) {
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.enum1"))), L"foo");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.enumBit1"))), L"bitFoo + bitBar");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.enum2"))), L"foo");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.enumBit2"))), L"bitFoo + bitBar");
} END_TEST


/**
 * Clone.
 */

BEGIN_TEST(CloneTest, BS) {
	CHECK(runFn<Bool>(S("tests.bs.testClone")));
	CHECK(runFn<Bool>(S("tests.bs.testCloneDerived")));
	CHECK(runFn<Bool>(S("tests.bs.testCloneValue")));
	CHECK_EQ(runFn<Int>(S("tests.bs.testCloneArray")), 10);
} END_TEST


/**
 * Generate.
 */

BEGIN_TEST(GenerateTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.genAdd"), 10, 20), 30);
	CHECK_EQ(runFn<Float>(S("tests.bs.genAdd"), 10.2f, 20.3f), 30.5f);
	CHECK_EQ(runFn<Int>(S("tests.bs.testGenClass"), 10), 12);
} END_TEST


/**
 * Patterns.
 */

BEGIN_TEST(PatternTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.testPattern")), 170);
	CHECK_EQ(runFn<Int>(S("tests.bs.testPatternNames")), 2010);

	CHECK_EQ(runFn<Int>(S("tests.bs.testPatternSplice1")), 23);
	CHECK_EQ(runFn<Int>(S("tests.bs.testPatternSplice2")), 6);

	// This is enforced by the syntax, but also by an explicit check in the code.
	CHECK_ERROR(runFn<void>(S("tests.bs.errors.testPatternOutside")), SyntaxError);
} END_TEST

/**
 * Lambda functions.
 */

BEGIN_TEST(LambdaTest, BS) {
	CHECK_EQ(runFn<Int>(S("tests.bs.testLambda")), 13);
	CHECK_EQ(runFn<Int>(S("tests.bs.testLambdaParam")), 15);
	CHECK_EQ(runFn<Int>(S("tests.bs.testLambdaVar")), 32);
	CHECK_EQ(runFn<Int>(S("tests.bs.testLambdaCapture")), 30);
	CHECK_EQ(runFn<Int>(S("tests.bs.testLambdaCapture2")), 53);
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.testLambdaMemory"))), L"[0, 1, 2, 0, 1, 3]");
} END_TEST


/**
 * Exception safety.
 */

BEGIN_TEST_FN(checkTimes, const wchar *name, nat times) {
	DbgVal::clear();
	CHECK_RUNS(runFn<void>(name, Int(0)));
	CHECK(DbgVal::clear());
	for (nat i = 0; i < times; i++) {
		CHECK_ERROR(runFn<void>(name, Int(i + 1)), DebugError);
		CHECK(DbgVal::clear());
	}
	CHECK_RUNS(runFn<void>(name, Int(times + 1)));
	CHECK(DbgVal::clear());
} END_TEST_FN

// Tests that checks the exception safety at various times in the generated code. Especially
// with regards to values.
BEGIN_TEST(BSException, BS) {
	CALL_TEST_FN(checkTimes, S("tests.bs.basicException"), 7);
	CALL_TEST_FN(checkTimes, S("tests.bs.fnException"), 3);
	CALL_TEST_FN(checkTimes, S("tests.bs.ctorError"), 8);
	CALL_TEST_FN(checkTimes, S("tests.bs.threadException"), 4);
} END_TEST

// Global variables.
BEGIN_TEST(Globals, BS) {
	Engine &e = gEngine();

	CHECK_EQ(runFn<Int>(S("tests.bs.testGlobal"), 10), 0);
	CHECK_EQ(runFn<Int>(S("tests.bs.testGlobal"), 5), 10);
	CHECK_EQ(runFn<Int>(S("tests.bs.testGlobal"), 7), 5);

	Str *strA = new (e) Str(S("A"));
	Str *strB = new (e) Str(S("B"));
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.testGlobal"), strA)), L"Hello");
	CHECK_EQ(runFn<Str *>(S("tests.bs.testGlobal"), strB), strA);
	CHECK_EQ(runFn<Str *>(S("tests.bs.testGlobal"), strA), strB);

	debug::DbgNoToS val;
	val.dummy = 1;
	CHECK_EQ(runFn<debug::DbgNoToS>(S("tests.bs.testGlobal"), val).dummy, 0);
	CHECK_EQ(runFn<debug::DbgNoToS>(S("tests.bs.testGlobal"), val).dummy, 1);

	// From other threads (would be good to check initialization is properly done as well).
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.threadGlobal"))), L"Other");

	CHECK_ERROR(runFn<Str *>(S("tests.bs.failThreadGlobal")), SyntaxError);

	// Make sure we have the proper initialization order.
	CHECK_EQ(toS(runFn<Str *>(S("tests.bs.getInitGlobal"))), L"Global: A");

} END_TEST

/**
 * Using a Variant from within Storm.
 */
BEGIN_TEST(VariantStormTest, BS) {
	CHECK_EQ(::toS(runFn<Variant>(S("tests.bs.createStrVariant")).get<Str *>()), L"test");
	CHECK_EQ(runFn<Variant>(S("tests.bs.createIntVariant")).get<Int>(), 15);

	// Using it together with RawPtr:
	CHECK(runFn<Bool>(S("tests.bs.variantRawObj")));
	CHECK(runFn<Bool>(S("tests.bs.variantRawInt")));

	CHECK_EQ(runFn<Variant>(S("tests.bs.variantFromRaw")).get<Int>(), 55);
	CHECK(runFn<Variant>(S("tests.bs.variantFromNull")).empty());
	CHECK_EQ(runFn<Int>(S("tests.bs.saveVariant")), 10);
} END_TEST

/**
 * Join behavior.
 */
BEGIN_TEST(JoinTest, BS) {
	Engine &e = gEngine();
	Array<Str *> *input = new (e) Array<Str *>();
	input->push(new (e) Str(S("A")));
	input->push(new (e) Str(S("B")));
	input->push(new (e) Str(S("C")));

	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.joinPlain"), input)), L"ABC");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.joinSep"), input)), L"A, B, C");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.joinTfm"), input)), L"test-Atest-Btest-Ctest2-Atest2-Btest2-C");
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.joinTfmSep"), input)), L"test-A, test-B, test-Ctest2-A, test2-B, test2-C");
} END_TEST

/**
 * Heavy tests.
 */

// Test the REPL of BS programmatically.
BEGIN_TESTX(ReplTest, BS) {
	runFn<void>(S("tests.bs.replTest"));
} END_TEST


BEGIN_TESTX(BFTest, BS) {
	// Takes a long time to run. Mostly here for testing.
	CHECK_RUNS(runFn<void>(S("tests.bf.separateBf")));
	CHECK_RUNS(runFn<void>(S("tests.bf.inlineBf")));
} END_TEST
