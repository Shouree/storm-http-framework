#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Compiler/Debug.h"

using namespace code;

BEGIN_TEST(RetPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);

	*l << prolog();

	*l << mov(ecx, intConst(100));

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 100);
} END_TEST

BEGIN_TEST(RetRefPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var t = l->createIntVar(l->root());

	*l << prolog();

	*l << mov(t, intConst(100));
	*l << lea(ptrA, t);

	*l << fnRetRef(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 100);
} END_TEST

BEGIN_TEST(RetFloatPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = floatDesc(e);

	*l << prolog();

	*l << mov(ecx, floatConst(10.0f));

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Float (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 10.0f);
} END_TEST

BEGIN_TEST(RetComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);

	Listing *l = new (e) Listing();
	l->result = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Var v = l->createVar(l->root(), dbgVal->size(), dbgVal->destructor()->ref(), freeDef | freePtr);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(dbgVal->defaultCtor()->ref(), false);

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef DbgVal (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)().v, 10);
	CHECK(DbgVal::clear());
} END_TEST

BEGIN_TEST(RetRefComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);

	Listing *l = new (e) Listing();
	l->result = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Var v = l->createVar(l->root(), dbgVal->size(), dbgVal->destructor()->ref(), freeDef | freePtr);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(dbgVal->defaultCtor()->ref(), false);

	*l << lea(ptrA, v);
	*l << fnRetRef(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef DbgVal (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)().v, 10);
	CHECK(DbgVal::clear());
} END_TEST

storm::debug::DbgVal STORM_FN createDbgVal() {
	return storm::debug::DbgVal(121);
}

BEGIN_TEST(RetCallComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);
	TypeDesc *dbgDesc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Ref toCall = arena->external(S("create"), address(&createDbgVal));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), dbgVal->size(), dbgVal->destructor()->ref(), freeDef | freePtr);

	*l << prolog();

	*l << fnCall(toCall, false, dbgDesc, v);

	// This is abusing the interface a tiny bit...
	*l << mov(eax, intRel(v, Offset()));

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)(), 121);
	CHECK(DbgVal::clear());

} END_TEST

BEGIN_TEST(RetCallRefComplex, Code) {
	using storm::debug::DbgVal;
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);
	TypeDesc *dbgDesc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());
	Ref toCall = arena->external(S("create"), address(&createDbgVal));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), dbgVal->size(), dbgVal->destructor()->ref(), freeDef | freePtr);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnCallRef(toCall, false, dbgDesc, ptrA);

	// This is abusing the interface a tiny bit...
	*l << mov(eax, intRel(v, Offset()));

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)(), 121);
	CHECK(DbgVal::clear());

} END_TEST

struct SimpleRet {
	size_t a;
	size_t b;

	// Make sure it is not a POD.
	SimpleRet(size_t a, size_t b) : a(a), b(b) {}
};

bool operator ==(const SimpleRet &a, const SimpleRet &b) {
	return a.a == b.a
		&& a.b == b.b;
}

bool operator !=(const SimpleRet &a, const SimpleRet &b) {
	return !(a == b);
}

wostream &operator <<(wostream &to, const SimpleRet &b) {
	return to << L"{ " << b.a << L", " << b.b << L" }";
}

SimpleDesc *retDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 2, 2);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	return desc;
}

BEGIN_TEST(RetSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = retDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 2);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << mov(ptrRel(ptrA, Offset()), ptrConst(10));
	*l << mov(ptrRel(ptrA, Offset::sPtr), ptrConst(20));

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef SimpleRet (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), SimpleRet(10, 20));
} END_TEST

SimpleRet CODECALL createSimple() {
	return SimpleRet(100, 20);
}

BEGIN_TEST(RetCallSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createSimple));

	Listing *l = new (e) Listing();
	l->result = ptrDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 2);

	*l << prolog();

	*l << fnCall(toCall, false, retDesc(e), v);
	*l << mov(ptrA, ptrRel(v, Offset()));
	*l << add(ptrA, ptrRel(v, Offset::sPtr));

	*l << fnRet(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef size_t (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 120);
} END_TEST

BEGIN_TEST(RetCallRefSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createSimple));

	Listing *l = new (e) Listing();
	l->result = ptrDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 2);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnCallRef(toCall, false, retDesc(e), ptrA);
	*l << mov(ptrA, ptrRel(v, Offset()));
	*l << add(ptrA, ptrRel(v, Offset::sPtr));

	*l << fnRet(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef size_t (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 120);
} END_TEST

struct SimpleFloatRet {
	int a, b;
	float c;

	// Make sure it is not a POD.
	SimpleFloatRet(int a, int b, float c) : a(a), b(b), c(c) {}
};

bool operator ==(const SimpleFloatRet &a, const SimpleFloatRet &b) {
	return a.a == b.a
		&& a.b == b.b
		&& a.c == b.c;
}

bool operator !=(const SimpleFloatRet &a, const SimpleFloatRet &b) {
	return !(a == b);
}

wostream &operator <<(wostream &to, const SimpleFloatRet &b) {
	return to << L"{ " << b.a << L", " << b.b << L", " << b.c << L" }";
}

SimpleDesc *retFloatDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sInt * 3, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sInt, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sInt, Offset::sInt);
	desc->at(2) = Primitive(primitive::real, Size::sInt, Offset::sInt * 2);
	return desc;
}

BEGIN_TEST(RetSimpleFloat, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = retFloatDesc(e);
	Var v = l->createVar(l->root(), Size::sInt * 3);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << mov(intRel(ptrA, Offset()), intConst(10));
	*l << mov(intRel(ptrA, Offset::sInt), intConst(20));
	*l << mov(intRel(ptrA, Offset::sInt * 2), floatConst(20.5f));

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef SimpleFloatRet (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), SimpleFloatRet(10, 20, 20.5f));
} END_TEST

SimpleFloatRet CODECALL createSimpleFloat() {
	return SimpleFloatRet(10, 20, 3.8f);
}

BEGIN_TEST(RetCallSimpleFloat, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createSimpleFloat));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), Size::sInt * 3);

	*l << prolog();

	*l << fnCall(toCall, false, retFloatDesc(e), v);
	*l << mov(eax, intRel(v, Offset()));
	*l << add(eax, intRel(v, Offset::sInt));

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 30);
} END_TEST

BEGIN_TEST(RetCallRefSimpleFloat, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createSimpleFloat));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), Size::sInt * 3);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnCallRef(toCall, false, retFloatDesc(e), ptrA);
	*l << mov(eax, intRel(v, Offset()));
	*l << add(eax, intRel(v, Offset::sInt));

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 30);
} END_TEST

// Struct that is too large to fit in the return registers on X86-64.
struct LargeSimpleRet {
	size_t a;
	size_t b;
	size_t c;

	// Make sure it is not a POD.
	LargeSimpleRet(size_t a, size_t b, size_t c) : a(a), b(b), c(c) {}

	// Used in tests.
	LargeSimpleRet CODECALL create() {
		return LargeSimpleRet(a + 8, b + 3, c + 7);
	}
};

bool operator ==(const LargeSimpleRet &a, const LargeSimpleRet &b) {
	return a.a == b.a
		&& a.b == b.b
		&& a.c == b.c;
}

bool operator !=(const LargeSimpleRet &a, const LargeSimpleRet &b) {
	return !(a == b);
}

wostream &operator <<(wostream &to, const LargeSimpleRet &a) {
	return to << L"{ " << a.a << L", " << a.b << L", " << a.c << L"}";
}

SimpleDesc *retLargeSimpleDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 3, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	desc->at(2) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr * 2);
	return desc;
}

BEGIN_TEST(RetLargeSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = retLargeSimpleDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 3);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << mov(ptrRel(ptrA, Offset()), ptrConst(10));
	*l << mov(ptrRel(ptrA, Offset::sPtr), ptrConst(20));
	*l << mov(ptrRel(ptrA, Offset::sPtr * 2), ptrConst(30));

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef LargeSimpleRet (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), LargeSimpleRet(10, 20, 30));
} END_TEST

BEGIN_TEST(RetRefLargeSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	l->result = retLargeSimpleDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 3);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << mov(ptrRel(ptrA, Offset()), ptrConst(10));
	*l << mov(ptrRel(ptrA, Offset::sPtr), ptrConst(20));
	*l << mov(ptrRel(ptrA, Offset::sPtr * 2), ptrConst(30));

	*l << lea(ptrA, v);
	*l << fnRetRef(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef LargeSimpleRet (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), LargeSimpleRet(10, 20, 30));
} END_TEST

static LargeSimpleRet createLargeSimple() {
	return LargeSimpleRet(20, 15, 22);
}

BEGIN_TEST(RetCallLargeSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createLargeSimple));

	Listing *l = new (e) Listing();
	l->result = ptrDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 3);

	*l << prolog();

	*l << fnCall(toCall, false, retLargeSimpleDesc(e), v);
	*l << mov(ptrA, ptrRel(v, Offset()));
	*l << sub(ptrA, ptrRel(v, Offset::sPtr));
	*l << add(ptrA, ptrRel(v, Offset::sPtr * 2));

	*l << fnRet(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef size_t (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 27);
} END_TEST

BEGIN_TEST(RetCallRefLargeSimple, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createLargeSimple));

	Listing *l = new (e) Listing();
	l->result = ptrDesc(e);
	Var v = l->createVar(l->root(), Size::sPtr * 3);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnCallRef(toCall, false, retLargeSimpleDesc(e), ptrA);
	*l << mov(ptrA, ptrRel(v, Offset()));
	*l << sub(ptrA, ptrRel(v, Offset::sPtr));
	*l << add(ptrA, ptrRel(v, Offset::sPtr * 2));

	*l << fnRet(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef size_t (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 27);
} END_TEST

/**
 * Is interaction with member functions working properly?
 */

BEGIN_TEST(RetLargeSimpleMember, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(true, retLargeSimpleDesc(e));
	Var v = l->createVar(l->root(), Size::sPtr * 3);
	Var me = l->createParam(ptrDesc(e));

	*l << prolog();

	*l << lea(ptrA, v);
	*l << mov(ptrC, me);
	*l << mov(ptrRel(ptrA, Offset()), ptrRel(ptrC, Offset()));
	*l << mov(ptrRel(ptrA, Offset::sPtr), ptrRel(ptrC, Offset::sPtr));
	*l << mov(ptrRel(ptrA, Offset::sPtr * 2), ptrRel(ptrC, Offset::sPtr * 2));

	*l << add(ptrRel(ptrA, Offset()), ptrConst(10));
	*l << add(ptrRel(ptrA, Offset::sPtr), ptrConst(20));
	*l << add(ptrRel(ptrA, Offset::sPtr * 2), ptrConst(30));

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef LargeSimpleRet (CODECALL LargeSimpleRet::*Fn)();
	Fn fn = asMemberPtr<Fn>(b->address());

	LargeSimpleRet original(1, 8, 3);
	CHECK_EQ((original.*fn)(), LargeSimpleRet(11, 28, 33));
} END_TEST

BEGIN_TEST(RetCallLargeSimpleMember, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&LargeSimpleRet::create));

	Listing *l = new (e) Listing(false, ptrDesc(e));
	Var v = l->createVar(l->root(), Size::sPtr * 3);
	Var me = l->createParam(ptrDesc(e));

	*l << prolog();

	*l << fnParam(ptrDesc(e), me);
	*l << fnCall(toCall, true, retLargeSimpleDesc(e), v);
	*l << mov(ptrA, ptrRel(v, Offset()));
	*l << sub(ptrA, ptrRel(v, Offset::sPtr));
	*l << add(ptrA, ptrRel(v, Offset::sPtr * 2));

	*l << fnRet(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef size_t (*Fn)(LargeSimpleRet *);
	Fn fn = (Fn)b->address();

	LargeSimpleRet original(2, 3, 4);
	CHECK_EQ((*fn)(&original), 10 - 6 + 11);
} END_TEST


/**
 * Doubles.
 */

BEGIN_TEST(RetDouble, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, doubleDesc(e));
	Var v = l->createVar(l->root(), Size::sDouble);

	*l << prolog();

	*l << mov(v, doubleConst(12.1));
	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef double (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 12.1);
} END_TEST

BEGIN_TEST(RetDoubleRef, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, doubleDesc(e));
	Var v = l->createVar(l->root(), Size::sDouble);

	*l << prolog();

	*l << mov(v, doubleConst(12.1));
	*l << lea(ptrA, v);
	*l << fnRetRef(ptrA);

	Binary *b = new (e) Binary(arena, l);
	typedef double (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 12.1);
} END_TEST

static double CODECALL createDouble() {
	return 14.1;
}

BEGIN_TEST(RetCallDouble, Code) {
	TODO(L"ENABLE");
	break;

	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createDouble));

	Listing *l = new (e) Listing(false, longDesc(e));
	Var v = l->createVar(l->root(), Size::sDouble);

	*l << prolog();

	*l << fnCall(toCall, false, doubleDesc(e), v);
	*l << fld(v);
	*l << fistp(v);

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef Long (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 14);
} END_TEST

BEGIN_TEST(RetCallRefDouble, Code) {
	TODO(L"ENABLE");
	break;

	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Ref toCall = arena->external(S("create"), address(&createDouble));

	Listing *l = new (e) Listing(false, longDesc(e));
	Var v = l->createVar(l->root(), Size::sDouble);

	*l << prolog();

	*l << lea(ptrA, v);
	*l << fnCallRef(toCall, false, doubleDesc(e), ptrA);
	*l << fld(v);
	*l << fistp(v);

	*l << fnRet(v);

	Binary *b = new (e) Binary(arena, l);
	typedef Long (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 14);
} END_TEST
