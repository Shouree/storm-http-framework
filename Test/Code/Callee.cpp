#include "stdafx.h"
#include "CallCommon.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Compiler/Debug.h"
#include "Core/Geometry/Rect.h"

/**
 * File containing tests for calling conventions from the callee's perspective.
 *
 * Mirrors tests in Call.cpp.
 */

using namespace code;

using storm::debug::DbgVal;

BEGIN_TEST(CalleePrimitive, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, intDesc(e));
	Var p1 = l->createParam(intDesc(e));

	*l << prolog();

	*l << mov(eax, p1);
	*l << add(eax, intConst(2));

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(100), 102);

} END_TEST


BEGIN_TEST(CalleePrimitiveMany, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, intDesc(e));
	Var p1 = l->createParam(intDesc(e));
	Var p2 = l->createParam(intDesc(e));
	Var p3 = l->createParam(intDesc(e));
	Var p4 = l->createParam(intDesc(e));
	Var p5 = l->createParam(intDesc(e));
	Var p6 = l->createParam(intDesc(e));
	Var p7 = l->createParam(intDesc(e));
	Var p8 = l->createParam(intDesc(e));
	Var p9 = l->createParam(intDesc(e));

	*l << prolog();

	*l << mov(eax, p1);
	*l << mul(eax, intConst(10));
	*l << add(eax, p2);
	*l << mul(eax, intConst(10));
	*l << add(eax, p3);
	*l << mul(eax, intConst(10));
	*l << add(eax, p4);
	*l << mul(eax, intConst(10));
	*l << add(eax, p5);
	*l << mul(eax, intConst(10));
	*l << add(eax, p6);
	*l << mul(eax, intConst(10));
	*l << add(eax, p7);
	*l << mul(eax, intConst(10));
	*l << add(eax, p8);
	*l << mul(eax, intConst(10));
	*l << add(eax, p9);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int, Int, Int, Int, Int, Int, Int, Int);
	Fn fn = (Fn)b->address();

	(*fn)(1, 2, 3, 4, 5, 6, 7, 8, 9);
	CHECK_EQ((*fn)(1, 2, 3, 4, 5, 6, 7, 8, 9), 123456789);

} END_TEST


BEGIN_TEST(CalleeTinyInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, intDesc(e));
	Var p1 = l->createParam(tinyIntDesc(e));

	*l << prolog();

	*l << mov(ebx, intRel(p1, Offset()));
	*l << sub(ebx, intRel(p1, Offset::sInt));

	*l << fnRet(ebx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(TinyIntParam);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(TinyIntParam(20, 10)), 10);

} END_TEST


BEGIN_TEST(CalleeSmallInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, intDesc(e));
	Var p1 = l->createParam(smallIntDesc(e));

	*l << prolog();

	// Note: Members are technically pointer-sized, so this is a bit incorrect, but good enough for a test.
	*l << mov(ebx, intRel(p1, Offset()));
	*l << sub(ebx, intRel(p1, Offset::sPtr));

	*l << fnRet(ebx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(SmallIntParam);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(SmallIntParam(20, 10)), 10);

} END_TEST


BEGIN_TEST(CalleeMixedInt, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, intDesc(e));
	Var p1 = l->createParam(largeIntDesc(e));
	Var p2 = l->createParam(smallIntDesc(e));

	*l << prolog();

	// Note: Members are technically pointer-sized, so this is a bit incorrect, but good enough for a test.
	*l << mov(ebx, intRel(p1, Offset()));
	*l << add(ebx, intRel(p1, Offset::sPtr));
	*l << sub(ebx, intRel(p1, Offset::sPtr * 2));

	*l << add(ebx, intRel(p2, Offset()));
	*l << sub(ebx, intRel(p2, Offset::sPtr));

	*l << fnRet(ebx);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(LargeIntParam, SmallIntParam);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(LargeIntParam(1, 2, 3), SmallIntParam(40, 10)), 30);
} END_TEST


BEGIN_TEST(CalleeMixed, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, floatDesc(e));
	Var p1 = l->createParam(mixedDesc(e));

	*l << prolog();

	// Implements: Float(a.a) - a.b / a.c
	*l << mov(ebx, intRel(p1, Offset()));
	*l << icastf(ebx, ebx);
	*l << mov(eax, floatRel(p1, Offset::sPtr));
	*l << fdiv(eax, floatRel(p1, Offset::sPtr + Offset::sFloat));
	*l << code::fsub(ebx, eax);

	*l << fnRet(ebx);

	Binary *b = new (e) Binary(arena, l);
	typedef Float (*Fn)(MixedParam);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(MixedParam(100, 40.0f, 10.0f)), 96.0f);

} END_TEST


BEGIN_TEST(CalleeComplex, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);
	Type *dbgVal = DbgVal::stormType(e);
	ComplexDesc *desc = new (e) ComplexDesc(dbgVal->size(), dbgVal->copyCtor()->ref(), dbgVal->destructor()->ref());

	Listing *l = new (e) Listing(false, intDesc(e));
	Var p1 = l->createParam(desc);
	Var p2 = l->createParam(intDesc(e));

	*l << prolog();

	*l << mov(eax, intRel(p1, Offset()));
	*l << add(eax, p2);

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(DbgVal, Int);
	Fn fn = (Fn)b->address();

	DbgVal::clear();
	{
		DbgVal v;
		CHECK_EQ((*fn)(v, 8), 18);
	}
	CHECK(DbgVal::clear());

} END_TEST


// Note: CallBytes in call.cpp tests receiving a struct as well.

BEGIN_TEST(CalleeIntFloat, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, floatDesc(e));
	Var p1 = l->createParam(floatDesc(e));
	Var p2 = l->createParam(intDesc(e));
	Var p3 = l->createParam(floatDesc(e));
	Var p4 = l->createParam(intDesc(e));

	*l << prolog();
	*l << mov(eax, p1);
	*l << icastf(ecx, p2);
	*l << code::fadd(eax, ecx);
	*l << code::fadd(eax, p3);
	*l << icastf(ecx, p4);
	*l << code::fadd(eax, ecx);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Float(*Fn)(Float, Int, Float, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(1.5f, 2, 2.5f, 3), 9.0f);
} END_TEST

BEGIN_TEST(CalleePoint, Code) {
	using geometry::Point;

	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	SimpleDesc *point = pointDesc(e);
	Listing *l = new (e) Listing(false, floatDesc(e));
	Var p = l->createParam(point);

	*l << prolog();
	*l << mov(eax, floatRel(p, Offset::sFloat));
	*l << fsub(eax, floatRel(p, Offset()));
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Float (*Fn)(Point);
	Fn fn = (Fn)b->address();

	Float r = (*fn)(Point(10, 20));
	CHECK_EQ(r, 10.0f);
} END_TEST


BEGIN_TEST(CalleePointRet, Code) {
	using geometry::Point;

	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	SimpleDesc *point = pointDesc(e);
	Listing *l = new (e) Listing(false, point);
	Var p = l->createVar(l->root(), point->size());

	*l << prolog();
	*l << mov(floatRel(p, Offset()), floatConst(3.0f));
	*l << mov(floatRel(p, Offset::sFloat), floatConst(4.0f));
	*l << fnRet(p);

	Binary *b = new (e) Binary(arena, l);
	typedef geometry::Point (*Fn)();
	Fn fn = (Fn)b->address();

	geometry::Point r = (*fn)();
	CHECK_EQ(r, Point(3, 4));
} END_TEST


BEGIN_TEST(CalleeRect, Code) {
	using geometry::Rect;

	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	SimpleDesc *rect = rectDesc(e);
	Listing *l = new (e) Listing(false, floatDesc(e));
	Var p = l->createParam(rect);

	*l << prolog();
	*l << mov(eax, floatRel(p, Offset::sFloat*2));
	*l << fsub(eax, floatRel(p, Offset()));
	*l << fadd(eax, floatRel(p, Offset::sFloat*3));
	*l << fsub(eax, floatRel(p, Offset::sFloat));
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Float (*Fn)(Rect);
	Fn fn = (Fn)b->address();

	Float r = (*fn)(Rect(10, 11, 21, 31));
	CHECK_EQ(r, 31.0f);
} END_TEST


BEGIN_TEST(CalleeRectRet, Code) {
	using geometry::Rect;

	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	SimpleDesc *rect = rectDesc(e);
	Listing *l = new (e) Listing(false, rect);
	Var p = l->createVar(l->root(), rect->size());

	*l << prolog();
	*l << mov(floatRel(p, Offset()), floatConst(1.0f));
	*l << mov(floatRel(p, Offset::sFloat), floatConst(2.0f));
	*l << mov(floatRel(p, Offset::sFloat*2), floatConst(3.0f));
	*l << mov(floatRel(p, Offset::sFloat*3), floatConst(4.0f));
	*l << fnRet(p);

	Binary *b = new (e) Binary(arena, l);
	typedef geometry::Rect (*Fn)();
	Fn fn = (Fn)b->address();

	geometry::Rect r = (*fn)();
	CHECK_EQ(r, Rect(1, 2, 3, 4));
} END_TEST
