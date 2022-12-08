#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(DivI, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << idiv(p1, p2);
	*l << mov(eax, p1);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(4, 2), 2);
	CHECK_EQ((*fn)(-8, 2), -4);
	CHECK_EQ((*fn)(8, -2), -4);
	CHECK_EQ((*fn)(-8, -2), 4);

	CHECK_ERROR((*fn)(8, 0), DivisionByZero);
} END_TEST

BEGIN_TEST(DivI2, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << mov(eax, p1);
	*l << mov(ebx, p2);
	*l << idiv(eax, ebx);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(4, 2), 2);
	CHECK_EQ((*fn)(-8, 2), -4);
	CHECK_EQ((*fn)(8, -2), -4);
	CHECK_EQ((*fn)(-8, -2), 4);
} END_TEST

BEGIN_TEST(DivU, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << udiv(p1, p2);
	*l << mov(eax, p1);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Nat (*Fn)(Nat, Nat);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(4, 2), 2);
	CHECK_EQ((*fn)(0x80000000, 2), 0x40000000);

} END_TEST

BEGIN_TEST(ModI, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << imod(p1, p2);
	*l << mov(eax, p1);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int, Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(18, 10), 8);
	CHECK_EQ((*fn)(-18, 10), -8);
	CHECK_EQ((*fn)(18, -10), 8);
	CHECK_EQ((*fn)(-18, -10), -8);

} END_TEST

BEGIN_TEST(ModU, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();
	Var p2 = l->createIntParam();

	*l << prolog();

	*l << umod(p1, p2);
	*l << mov(eax, p1);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Nat (*Fn)(Nat, Nat);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(14, 10), 4);
	CHECK_EQ((*fn)(0x80000000, 3), 2);

} END_TEST

BEGIN_TEST(DivIConst, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();

	*l << prolog();

	*l << idiv(p1, intConst(-2));
	*l << mov(eax, p1);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(8), -4);
	CHECK_EQ((*fn)(-8), 4);

} END_TEST

BEGIN_TEST(ModIConst, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createIntParam();

	*l << prolog();

	*l << imod(p1, intConst(-10));
	*l << mov(eax, p1);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(18), 8);
	CHECK_EQ((*fn)(-18), -8);

} END_TEST


BEGIN_TEST(DivByte, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createByteParam();
	Var p2 = l->createByteParam();

	*l << prolog();

	*l << udiv(p1, p2);
	*l << mov(al, p1);

	l->result = byteDesc(e);
	*l << fnRet(al);

	Binary *b = new (e) Binary(arena, l);
	typedef Byte (*Fn)(Byte, Byte);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(8, 2), 4);
	CHECK_EQ((*fn)(0x80, 2), 0x40);

} END_TEST

BEGIN_TEST(ModByte, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createByteParam();
	Var p2 = l->createByteParam();

	*l << prolog();

	*l << umod(p1, p2);
	*l << mov(al, p1);

	l->result = byteDesc(e);
	*l << fnRet(al);

	Binary *b = new (e) Binary(arena, l);
	typedef Byte (*Fn)(Byte, Byte);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(18, 10), 8);

} END_TEST
