#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Compiler/Debug.h"

using namespace code;

static Int destroyed = 0;

static void intCleanup(Int v) {
	destroyed += v;
}

static void intCleanupGc(Int v) {
	// Run a GC cycle to make interesting things happen during unwinding.
	// gEngine().gc.collect();
	// Instead of causing a full collect (which takes ~500 ms on Arm) we allocate some objects.
	storm::debug::createList(gEngine(), 1024);
	destroyed += v;
}

static void intPtrCleanup(Int *v) {
	destroyed += *v;
}

static Int throwAt = 0;

static Int throwError(Int point) {
	if (point == throwAt) {
		throw Error();
	}
	return point;
}

BEGIN_TEST(CodeExceptionTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intCleanup = arena->external(S("intCleanup"), address(&::intCleanup));
	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	Block block = l->createBlock(l->root());
	Var v = l->createVar(l->root(), Size::sInt, intCleanup, freeOnException);
	Var w = l->createVar(block, Size::sInt, intCleanup, freeOnException);

	*l << prolog();

	*l << mov(v, intConst(10));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn, false);

	*l << begin(block);
	*l << mov(w, intConst(20));
	*l << fnParam(intDesc(e), intConst(2));
	*l << fnCall(errorFn, false);
	*l << end(block);

	*l << fnParam(intDesc(e), intConst(3));
	*l << fnCall(errorFn, false);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	throwAt = 1;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 10);

	throwAt = 2;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 30);

	throwAt = 3;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 10);

	throwAt = 4;
	destroyed = 0;
	CHECK_RUNS((*fn)());
	CHECK_EQ(destroyed, 0);

} END_TEST


BEGIN_TEST(CodeCleanupTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intCleanup = arena->external(S("intCleanup"), address(&::intCleanup));
	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	Block block = l->createBlock(l->root());
	Var v = l->createVar(l->root(), Size::sInt, intCleanup);
	Var w = l->createVar(block, Size::sInt, intCleanup);
	Var y = l->createVar(l->root(), Size::sInt, intCleanup, code::freeDef | code::freeInactive);

	*l << prolog();

	*l << mov(v, intConst(10));
	*l << mov(y, intConst(30));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn, false);

	*l << begin(block);
	*l << mov(w, intConst(20));
	*l << fnParam(intDesc(e), intConst(2));
	*l << fnCall(errorFn, false);

	*l << activate(y);
	*l << fnParam(intDesc(e), intConst(3));
	*l << fnCall(errorFn, false);

	*l << mov(w, intConst(10));
	*l << end(block);

	*l << fnParam(intDesc(e), intConst(4));
	*l << fnCall(errorFn, false);

	*l << mov(y, intConst(20));
	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	throwAt = 1;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 10);

	throwAt = 2;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 30);

	throwAt = 3;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 60);

	throwAt = 4;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 50);

	throwAt = 5;
	destroyed = 0;
	CHECK_RUNS((*fn)());
	CHECK_EQ(destroyed, 40);

} END_TEST


BEGIN_TEST(ExceptionRefTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intCleanup = arena->external(S("intCleanup"), address(&::intPtrCleanup));
	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	Var v = l->createVar(l->root(), Size::sInt, intCleanup, freeOnBoth | freePtr);

	*l << prolog();

	*l << mov(v, intConst(103));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn, false);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	throwAt = 1;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
	CHECK_EQ(destroyed, 103);

	throwAt = 2;
	destroyed = 0;
	CHECK_RUNS((*fn)());
	CHECK_EQ(destroyed, 103);

} END_TEST


/**
 * See that we properly restore seh frames and similar things.
 */
#if defined(X86) && defined(WINDOWS)

BEGIN_TEST(ExceptionSehTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intCleanup = arena->external(S("intCleanup"), &::intCleanup);
	Ref errorFn = arena->external(S("errorFn"), &::throwError);

	Listing *l = new (e) Listing();
	Block block = l->createBlock(l->root());
	Var v = l->createVar(l->root(), Size::sInt, intCleanup);

	*l << prolog();

	*l << mov(v, intConst(103));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn, false, intDesc(e), eax);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	void *sehTop, *after;
	__asm {
		mov eax, fs:[0];
		mov sehTop, eax;
	};

	throwAt = 0;
	destroyed = 0;
	CHECK_RUNS((*fn)());

	__asm {
		mov eax, fs:[0];
		mov after, eax;
	};

	CHECK_EQ(after, sehTop);

	// Crashes if the above check fails...
	throwAt = 1;
	destroyed = 0;
	CHECK_ERROR((*fn)(), Error);
} END_TEST

#endif


struct Large {
	int a;
	int b;
	int c;

	Large() : a(0), b(10), c(0) {}
	~Large();
};

static bool correct = false;
static int times = 0;

static void destroyLarge(Large *large) {
	correct &= large->a == 0;
	correct &= large->b == 10;
	correct &= large->c == 0;
	times++;
}

Large::~Large() {
	destroyLarge(this);
}

static void copyLarge(Large *dest, Large *src) {
	*dest = *src;
}

BEGIN_TEST(ExceptionLargeTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref largeCopy = arena->external(S("largeCopy"), address(&::copyLarge));
	Ref largeCleanup = arena->external(S("largeCleanup"), address(&::destroyLarge));
	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	ComplexDesc *desc = new (e) ComplexDesc(Size::sInt * 3, largeCopy, largeCleanup);
	Var p = l->createParam(desc);
	Var v = l->createVar(l->root(), Size::sInt * 3, largeCleanup, freeOnBoth | freePtr);

	*l << prolog();

	*l << mov(intRel(v, Offset()), intRel(p, Offset()));
	*l << mov(intRel(v, Offset::sInt), intRel(p, Offset::sInt));
	*l << mov(intRel(v, Offset::sInt * 2), intRel(p, Offset::sInt * 2));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnCall(errorFn, false);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Large);
	Fn fn = (Fn)b->address();

	Large large;

	throwAt = 1;
	correct = true;
	times = 0;
	CHECK_ERROR((*fn)(large), Error);
	CHECK(correct);
	CHECK_EQ(times, 2);

	throwAt = 2;
	correct = true;
	times = 0;
	CHECK_RUNS((*fn)(large));
	CHECK(correct);
	CHECK_EQ(times, 2);

} END_TEST


BEGIN_TEST(ExceptionLayers, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref errorFn = arena->external(S("errorFn"), address(&::throwError));

	Listing *l = new (e) Listing();
	*l << prolog();
	*l << mov(ebx, intConst(1));
	*l << fnParam(intDesc(e), ebx);
	*l << fnCall(errorFn, false);
	*l << fnRet();

	Binary *b = new (e) Binary(arena, l);
	RefSource *next = new (e) StrRefSource(new (e) Str(S("level2")), b);

	l = new (e) Listing();
	*l << prolog();
	*l << mov(ebx, intConst(0));
	*l << fnCall(Ref(next), false);
	*l << fnRet();

	b = new (e) Binary(arena, l);
	typedef void (*Fn)();
	Fn fn = (Fn)b->address();

	throwAt = 1;
	CHECK_ERROR((*fn)(), Error);

} END_TEST

static void CODECALL throwPtr(Nat i) {
	// To make sure destructors in this frame are executed properly!
	debug::DbgVal dtorCheck;

	TODO(L"Stack traces inside Storm functions don't seem to work properly on 64-bit Windows...");
	// PVAR(collectStackTrace(gEngine()).format());

	if (i == 1) {
		Str *obj = new (gEngine()) Str(S("Throw me!"));
		// PLN(L"Throwing " << (void *)obj << L", " << obj);
		throw obj;
	} else if (i == 2 || i == 10) {
		StrBuf *buf = new (gEngine()) StrBuf();
		*buf << S("Buffer");
		// PLN(L"Throwing " << (void *)buf << L", " << buf);
		throw buf;
	} else if (i == 20) {
		// Constant object.
		const Str *obj = new (gEngine()) Str(S("Throw const!"));
		throw obj;
	}
}

static StrBuf *CODECALL addBuf(StrBuf *to) {
	*to << S("!");
	return to;
}

BEGIN_TEST_(ExceptionCatch, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref errorFn = arena->external(S("errorFn"), address(&::throwPtr));
	Ref appendFn = arena->external(S("appendFn"), address(&::addBuf));
	Ref freeInt = arena->external(S("freeInt"), address(&::intCleanupGc));

	Listing *z = new (e) Listing(false);
	Var p = z->createParam(intDesc(e));
	Var w = z->createVar(z->root(), Size::sInt, freeInt);
	*z << prolog();
	// *z << mov(w, intConst(3));
	*z << fnParam(intDesc(e), p);
	*z << fnCall(errorFn, false);
	*z << fnRet();

	Binary *q = new (e) Binary(arena, z, true);


	Listing *l = new (e) Listing(false, ptrDesc(e));

	Var tmp = l->createVar(l->root(), Size::sPtr);
	Var param = l->createParam(intDesc(e));

	Label caught1 = l->label();
	Label caught2 = l->label();

	Block block = l->createBlock(l->root());
	l->addCatch(block, StormInfo<StrBuf>::type(e), caught1);
	l->addCatch(block, StormInfo<Object>::type(e), caught2);

	Var outerInt = l->createVar(l->root(), Size::sInt, freeInt);
	Var innerInt = l->createVar(block, Size::sInt, freeInt);

	*l << prolog();
	*l << mov(outerInt, intConst(8));
	*l << begin(block);
	*l << mov(innerInt, intConst(4));
	*l << fnParam(intDesc(e), param);
	// *l << fnCall(errorFn, false);
	RefSource *rs = new (e) StrRefSource(new (e) Str(S("Z")), q);
	*l << fnCall(Ref(rs), false);
	*l << end(block);
	*l << fnRet(ptrConst(0));

	// A StrBuf was caught.
	*l << caught1;
	*l << add(outerInt, intConst(10));
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(appendFn, false, ptrDesc(e), ptrA);

	// Generic object was caught.
	*l << caught2;

	*l << mov(tmp, ptrA);
	*l << mov(eax, param);
	*l << sub(eax, intConst(1));
	*l << fnParam(intDesc(e), eax);
	*l << fnCall(errorFn, false);

	*l << sub(outerInt, intConst(1));
	*l << fnRet(tmp);

	Binary *b = new (e) Binary(arena, l, true);
	typedef Object *(*Fn)(Int i);
	Fn fn = (Fn)b->address();

	using debug::DbgVal;

	PVAR(q->address());
	PVAR(b->address());

	// Make sure registers are preserved correctly when exceptions are thrown. This asserts if they
	// are not preserved properly, which might cause the remainder of this test to fail.
	callFn(b->address(), 1);

	PLN(L"--- Sanity check OK ---");

	DbgVal::clear();
	destroyed = 0;
	// Easy: No exception thrown.
	CHECK_EQ((*fn)(0), (Object *)null);
	CHECK(DbgVal::clear());
	CHECK_EQ(destroyed, 4 + 8);

	PLN(L"--- FIRST OK ---");

	// Throws an exception at first, then catches it as an object.
	destroyed = 0;
	CHECK_EQ(::toS((*fn)(1)), L"Throw me!");
	CHECK(DbgVal::clear());
	CHECK_EQ(destroyed, 4 + 7);

	PLN(L"--- SECOND OK ---");

	// Throws a StrBuf at first, then catches it as a StrBuf and adds an!
	destroyed = 0;
	CHECK_EQ(::toS((*fn)(10)), L"Buffer!");
	CHECK(DbgVal::clear());
	CHECK_EQ(destroyed, 4 + 17);

	PLN(L"--- FOURTH OK ---");

	// Throws a StrBuf, catches it, and throws another exception.
	destroyed = 0;
	CHECK_ERROR(::toS((*fn)(2)), Str *);
	CHECK(DbgVal::clear());
	CHECK_EQ(destroyed, 4 + 18);

	PLN(L"--- FOURTH OK ---");

	// Throws a const Str and tries to catch it.
	destroyed = 0;
	CHECK_EQ(::toS((*fn)(20)), L"Throw const!");
	CHECK(DbgVal::clear());
	CHECK_EQ(destroyed, 4 + 7);

	// // DebugBreak();
	// try {
	// 	Str *r = (*fn)();
	// 	PLN(L"Got result: " << (void *)r << L", " << r);
	// } catch (Str *s) {
	// 	PLN(L"Caught string: " << (void *)s << L", " << s);
	// }
} END_TEST


// Has many parameters, so that at least one of them is on the stack!
static void CODECALL throwStrError(Int, Int, Int, Int, Int, Int, Int, Int, Int, Int) {
	throw new (gEngine()) Str(S("Throw me!"));
}

#pragma warning(push)
// This warning is about returning the address of a local variable. In this case it is fine, as we
// only use it to measure the depth of the stack.
#pragma warning(disable: 4172)
size_t captureSp() {
	// This is approximative, but good enough for the test (i.e., we only want it to be at the same
	// location, we don't care exactly what part of the stack we get).
	volatile int x;
	return size_t(&x);
}
#pragma warning(pop)

BEGIN_TEST(ExceptionRestoreSpOnCatch, Code) {
	// Make sure that the stack pointer is restored properly if needed.
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref errorFn = arena->external(S("errorFn"), address(&::throwStrError));
	Ref captureSp = arena->external(S("captureSp"), address(&::captureSp));

	Listing *l = new (e) Listing();
	Var param1 = l->createParam(ptrDesc(e));
	Var param2 = l->createParam(ptrDesc(e));
	Block ehBlock = l->createBlock(l->root());
	Label catched = l->label();
	Label fnEnd = l->label();
	l->addCatch(ehBlock, StormInfo<Object>::type(e), catched);

	*l << prolog();
	*l << begin(ehBlock);
	*l << fnCall(captureSp, false, ptrDesc(e), ptrA);
	*l << mov(ptrB, param1);
	*l << mov(ptrRel(ptrB, Offset()), ptrA);
	*l << fnParam(intDesc(e), intConst(0));
	*l << fnParam(intDesc(e), intConst(1));
	*l << fnParam(intDesc(e), intConst(2));
	*l << fnParam(intDesc(e), intConst(3));
	*l << fnParam(intDesc(e), intConst(4));
	*l << fnParam(intDesc(e), intConst(5));
	*l << fnParam(intDesc(e), intConst(6));
	*l << fnParam(intDesc(e), intConst(7));
	*l << fnParam(intDesc(e), intConst(8));
	*l << fnParam(intDesc(e), intConst(9));
	*l << fnCall(errorFn, false);
	*l << end(ehBlock);
	*l << jmp(fnEnd);

	*l << catched;
	*l << fnCall(captureSp, false, ptrDesc(e), ptrA);
	*l << mov(ptrB, param2);
	*l << mov(ptrRel(ptrB, Offset()), ptrA);

	*l << fnEnd;
	*l << fnRet();

	Binary *b = new (e) Binary(arena, l);
	typedef void (*Fn)(size_t *a, size_t *b);
	Fn fn = (Fn)b->address();

	size_t before = 0;
	size_t after = 0;
	CHECK_RUNS((*fn)(&before, &after));
	CHECK_EQ(after, before);

} END_TEST
