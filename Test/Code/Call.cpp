#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Code/X64/Arena.h"
#include "Code/X64/Asm.h"
#include "Code/Arm64/Arena.h"
#include "Compiler/Debug.h"

using namespace code;

using storm::debug::DbgVal;

// If this is static, it seems the compiler optimizes it away, which breaks stuff.
Int CODECALL callIntFn(Int v) {
	return v + 2;
}

BEGIN_TEST(CallPrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	*l << prolog();

	*l << fnParam(intDesc(e), intConst(100));
	*l << fnCall(intFn, false, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 102);
} END_TEST

BEGIN_TEST(CallPrimitiveRef, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createIntVar(l->root());
	*l << prolog();

	*l << mov(v, intConst(100));
	*l << lea(ptrA, v);
	*l << fnParamRef(intDesc(e), ptrA);
	*l << fnCall(intFn, false, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 102);
} END_TEST

Int CODECALL callIntManyFn(Int a, Int b, Int c, Int d, Int e, Int f, Int g, Int h) {
	return a*10000000 + b*1000000 + c*100000 + d*10000 + e*1000 + f*100 + g*10 + h;
}

BEGIN_TEST(CallPrimitiveMany, Code) {
	// Attempts to pass enough parameters to put at least one on the stack. Put some parameters in
	// register so that the backend has to reorder the register assignment to some degree.
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntManyFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	*l << prolog();

	*l << mov(eax, intConst(1));
	*l << mov(ecx, intConst(2));
	*l << fnParam(intDesc(e), intConst(3));
	*l << fnParam(intDesc(e), intConst(4));
	*l << fnParam(intDesc(e), intConst(5));
	*l << fnParam(intDesc(e), intConst(6));
	*l << fnParam(intDesc(e), intConst(7));
	*l << fnParam(intDesc(e), ecx);
	*l << fnParam(intDesc(e), intConst(8));
	*l << fnParam(intDesc(e), eax);
	*l << fnCall(intFn, false, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 34567281);
} END_TEST

#ifdef X64

// Only usable on X86-64, as we rely on platform specific registers.
BEGIN_TEST(CallPrimitiveMany64, Code) {
	// Call a function with parameters forming a cycle that the backend needs to break in order to
	// properly assign the registers.
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callIntManyFn));

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	*l << prolog();

	using namespace code::x64;
	*l << mov(edi, intConst(1));
	*l << mov(esi, intConst(2));
	*l << mov(edx, intConst(3));
	*l << mov(ecx, intConst(4));
	*l << mov(e8,  intConst(5));
	*l << mov(e9,  intConst(6));

	*l << fnParam(intDesc(e), esi);
	*l << fnParam(intDesc(e), edx);
	*l << fnParam(intDesc(e), ecx);
	*l << fnParam(intDesc(e), e8);
	*l << fnParam(intDesc(e), e9);
	*l << fnParam(intDesc(e), edi);
	*l << fnParam(intDesc(e), intConst(0));
	*l << fnParam(intDesc(e), intConst(0));
	*l << fnCall(intFn, false, intDesc(e), ecx);

	*l << fnRet(ecx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 23456100);
} END_TEST

#endif

struct SmallIntParam {
	size_t a;
	size_t b;

	// Make sure it is not a POD.
	SmallIntParam(size_t a, size_t b) : a(a), b(b) {}
};

SimpleDesc *smallIntDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 2, 2);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	return desc;
}

Int CODECALL callSmallInt(SmallIntParam p) {
	return Int(p.a + p.b);
}

BEGIN_TEST(CallSmallInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callSmallInt));
	SimpleDesc *desc = smallIntDesc(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), desc->size());

	*l << prolog();

	*l << mov(ptrRel(v, Offset()), ptrConst(10));
	*l << mov(ptrRel(v, Offset::sPtr), ptrConst(40));

	*l << fnParam(desc, v);
	*l << fnCall(intFn, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 50);
} END_TEST

BEGIN_TEST(CallSmallIntRef, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callSmallInt));
	SimpleDesc *desc = smallIntDesc(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var v = l->createVar(l->root(), desc->size());

	*l << prolog();

	*l << mov(ptrRel(v, Offset()), ptrConst(10));
	*l << mov(ptrRel(v, Offset::sPtr), ptrConst(40));

	*l << lea(ptrA, v);
	*l << fnParamRef(desc, ptrA);
	*l << fnCall(intFn, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 50);
} END_TEST

struct LargeIntParam {
	size_t a;
	size_t b;
	size_t c;

	// Make sure it is not a POD.
	LargeIntParam(size_t a, size_t b, size_t c) : a(a), b(b), c(c) {}
};

SimpleDesc *largeIntDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr * 3, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr);
	desc->at(2) = Primitive(primitive::integer, Size::sPtr, Offset::sPtr * 2);
	return desc;
}

Int CODECALL callMixedInt(LargeIntParam a, SmallIntParam b) {
	return Int(a.a + a.b - a.c + b.a - b.b);
}

BEGIN_TEST(CallMixedInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("mixedIntFn"), address(&callMixedInt));
	SimpleDesc *small = smallIntDesc(e);
	SimpleDesc *large = largeIntDesc(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var a = l->createVar(l->root(), large->size());
	Var b = l->createVar(l->root(), small->size());

	*l << prolog();

	*l << mov(ptrRel(a, Offset()), ptrConst(1));
	*l << mov(ptrRel(a, Offset::sPtr), ptrConst(2));
	*l << mov(ptrRel(a, Offset::sPtr * 2), ptrConst(3));

	*l << mov(ptrRel(b, Offset()), ptrConst(40));
	*l << mov(ptrRel(b, Offset::sPtr), ptrConst(10));

	*l << fnParam(large, a);
	*l << fnParam(small, b);
	*l << fnCall(intFn, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)bin->address();
	CHECK_EQ((*fn)(), 30);
} END_TEST

BEGIN_TEST(CallMixedIntRef, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref intFn = arena->external(S("intFn"), address(&callMixedInt));
	SimpleDesc *small = smallIntDesc(e);
	SimpleDesc *large = largeIntDesc(e);

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var a = l->createVar(l->root(), large->size());
	Var b = l->createVar(l->root(), small->size());

	*l << prolog();

	*l << mov(ptrRel(a, Offset()), ptrConst(1));
	*l << mov(ptrRel(a, Offset::sPtr), ptrConst(2));
	*l << mov(ptrRel(a, Offset::sPtr * 2), ptrConst(3));

	*l << mov(ptrRel(b, Offset()), ptrConst(40));
	*l << mov(ptrRel(b, Offset::sPtr), ptrConst(10));

	*l << lea(ptrA, a);
	*l << lea(ptrB, b);

	*l << fnParamRef(large, ptrA);
	*l << fnParamRef(small, ptrB);
	*l << fnCall(intFn, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 30);
} END_TEST

struct MixedParam {
	size_t a;
	Float b;
	Float c;

	// Make sure it is not a POD.
	MixedParam(size_t a, Float b, Float c) : a(a), b(b), c(c) {}
};

SimpleDesc *mixedDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sPtr + Size::sFloat * 2, 3);
	desc->at(0) = Primitive(primitive::integer, Size::sPtr, Offset());
	desc->at(1) = Primitive(primitive::real, Size::sFloat, Offset::sPtr);
	desc->at(2) = Primitive(primitive::real, Size::sFloat, Offset::sPtr + Offset::sFloat);
	return desc;
}

Float CODECALL callMixed(MixedParam a) {
	return Float(a.a) - a.b / a.c;
}

BEGIN_TEST(CallMixed, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref mixedFn = arena->external(S("mixedFn"), address(&callMixed));
	SimpleDesc *mixed = mixedDesc(e);

	Listing *l = new (e) Listing();
	l->result = floatDesc(e);
	Var a = l->createVar(l->root(), mixed->size());

	*l << prolog();

	*l << mov(ptrRel(a, Offset()), ptrConst(100));
	*l << mov(floatRel(a, Offset::sPtr), floatConst(40.0f));
	*l << mov(floatRel(a, Offset::sPtr + Offset::sFloat), floatConst(10.0f));

	*l << fnParam(mixed, a);
	*l << fnCall(mixedFn, false, floatDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Float (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 96.0f);
} END_TEST

BEGIN_TEST(CallMixedRef, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref mixedFn = arena->external(S("mixedFn"), address(&callMixed));
	SimpleDesc *mixed = mixedDesc(e);

	Listing *l = new (e) Listing();
	l->result = floatDesc(e);
	Var a = l->createVar(l->root(), mixed->size());

	*l << prolog();

	*l << mov(ptrRel(a, Offset()), ptrConst(100));
	*l << mov(floatRel(a, Offset::sPtr), floatConst(40.0f));
	*l << mov(floatRel(a, Offset::sPtr + Offset::sFloat), floatConst(10.0f));

	*l << lea(ptrA, a);
	*l << fnParamRef(mixed, ptrA);
	*l << fnCall(mixedFn, false, floatDesc(e), eax);

	*l << fnRet(eax);

	Binary *bin = new (e) Binary(arena, l);
	typedef Float (*Fn)();
	Fn fn = (Fn)bin->address();

	CHECK_EQ((*fn)(), 96.0f);
} END_TEST

Int CODECALL callComplex(DbgVal dbg, Int v) {
	return dbg.v + v;
}

BEGIN_TEST(CallComplex, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);

	Ref toCall = arena->external(S("callComplex"), address(&callComplex));
	ComplexDesc *desc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var a = l->createVar(l->root(), desc);

	*l << prolog();

	*l << lea(ptrA, a);
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(dbgVal->defaultCtor()->ref(), false);

	*l << mov(ecx, intConst(8));

	*l << fnParam(desc, a);
	*l << fnParam(intDesc(e), ecx);
	*l << fnCall(toCall, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)(), 18);
	CHECK(DbgVal::clear());
} END_TEST

BEGIN_TEST(CallRefComplex, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);

	Ref toCall = arena->external(S("callComplex"), address(&callComplex));
	ComplexDesc *desc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());

	Listing *l = new (e) Listing();
	l->result = intDesc(e);
	Var a = l->createVar(l->root(), desc);

	*l << prolog();

	*l << lea(ptrA, a);
	*l << fnParam(ptrDesc(e), ptrA);
	*l << fnCall(dbgVal->defaultCtor()->ref(), false);

	*l << mov(ecx, intConst(8));
	*l << lea(ptrA, a);
	*l << fnParamRef(desc, ptrA);
	*l << fnParam(intDesc(e), ecx);
	*l << fnCall(toCall, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	CHECK_EQ((*fn)(), 18);
	CHECK(DbgVal::clear());
} END_TEST


static Int CODECALL intFn(Int a, Int b, Int c) {
	return 100*a + 10*b + c;
}

// Try loading values for the call from an array, to make sure that registers are preserved properly.
BEGIN_TEST(CallFromArray, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	TypeDesc *valDesc = intDesc(e);

	Ref toCall = arena->external(S("intFn"), address(&intFn));

	Listing *l = new (e) Listing(false, valDesc);
	Var params = l->createParam(ptrDesc(e));

	*l << prolog();

	// Use a 'bad' register for the current backend. ptrDi on X64, ptrA on ARM, no particular on x86.
	Reg reg = ptrA;
	if (as<code::x64::Arena>(arena))
		reg = code::x64::ptrDi;
	if (as<code::arm64::Arena>(arena))
		reg = ptrA;

	*l << mov(reg, params);
	*l << fnParamRef(valDesc, ptrRel(reg, Offset()));
	*l << fnParamRef(valDesc, ptrRel(reg, Offset::sPtr));
	*l << fnParamRef(valDesc, ptrRel(reg, Offset::sPtr*2));

	*l << fnCall(toCall, false, valDesc, eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(const Int **);
	Fn fn = (Fn)b->address();

	const Int va = 5, vb = 7, vc = 2;
	const Int *actuals[] = { &va, &vb, &vc };
	CHECK_EQ((*fn)(actuals), 572);

} END_TEST

static Int CODECALL complexIntFn(DbgVal a, DbgVal b, DbgVal c) {
	return 100*a.get() + 10*b.get() + c.get();
}

BEGIN_TEST(CallComplexFromArray, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);
	ComplexDesc *valDesc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());

	Ref toCall = arena->external(S("complexIntFn"), address(&complexIntFn));

	Listing *l = new (e) Listing(false, intDesc(e));
	Var params = l->createParam(ptrDesc(e));

	*l << prolog();

	*l << mov(ptrA, params);
	*l << fnParamRef(valDesc, ptrRel(ptrA, Offset()));
	*l << fnParamRef(valDesc, ptrRel(ptrA, Offset::sPtr));
	*l << fnParamRef(valDesc, ptrRel(ptrA, Offset::sPtr*2));

	*l << fnCall(toCall, false, intDesc(e), eax);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(const DbgVal **);
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	{
		const DbgVal va(5), vb(7), vc(2);
		const DbgVal *actuals[] = { &va, &vb, &vc };
		CHECK_EQ((*fn)(actuals), 572);
	}
	CHECK(DbgVal::clear());

} END_TEST

struct ByteStruct {
	Byte a;
	Byte b;

	ByteStruct(Byte a, Byte b) : a(a), b(b) {}
};

SimpleDesc *bytesDesc(Engine &e) {
	SimpleDesc *desc = new (e) SimpleDesc(Size::sByte * 2, 2);
	desc->at(0) = Primitive(primitive::integer, Size::sByte, Offset());
	desc->at(1) = Primitive(primitive::integer, Size::sByte, Offset::sByte);
	return desc;
}

ByteStruct CODECALL callBytes(ByteStruct p) {
	return ByteStruct(p.a + 10, p.b - 5);
}

BEGIN_TEST(CallBytes, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Ref byteFn = arena->external(S("byteFn"), address(&callBytes));
	SimpleDesc *bytes = bytesDesc(e);

	Listing *l = new (e) Listing(false, bytes);
	Var p = l->createParam(bytes);
	Var a = l->createVar(l->root(), bytes->size());
	Var b = l->createVar(l->root(), bytes->size());

	*l << prolog();
	*l << mov(byteRel(a, Offset()), byteRel(p, Offset()));
	*l << mov(byteRel(a, Offset::sByte), byteRel(p, Offset::sByte));

	*l << add(byteRel(a, Offset()), byteConst(21));
	*l << add(byteRel(a, Offset::sByte), byteConst(18));

	*l << fnParam(bytes, a);
	*l << fnCall(byteFn, false, bytes, b);

	*l << fnRet(b);

	Binary *bin = new (e) Binary(arena, l);
	typedef ByteStruct (*Fn)(ByteStruct);
	Fn fn = (Fn)bin->address();

	ByteStruct r = (*fn)(ByteStruct(2, 3));
	CHECK_EQ(r.a, 33);
	CHECK_EQ(r.b, 16);
} END_TEST

BEGIN_TEST(CallMore, Code) {
	TODO(L"Implement more tests!");
	// The following tests are needed:
	// - Function call that has complex parameters, then uses value in eax (probably crashes x64 backend as well)
	// - Receiving simple and complex parameters that get passed in memory.
} END_TEST
