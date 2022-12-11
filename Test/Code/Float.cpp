#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(FloatTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createFloatParam();
	Var p2 = l->createFloatParam();
	Var v1 = l->createIntVar(l->root());

	*l << prolog();

	*l << mov(eax, p1);
	*l << fmul(eax, p2);
	*l << mov(v1, intConst(10));
	*l << icastf(ebx, v1);
	*l << fmul(Operand(eax), ebx);
	*l << fcasti(eax, eax);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Float, Float);
	Fn fn = (Fn)b->address();
	// Note: the result is 270.6 truncated to 270.
	CHECK_EQ((*fn)(12.3f, 2.2f), 270);

} END_TEST

BEGIN_TEST(FloatConstTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	*l << prolog();

	*l << mov(eax, floatConst(10.2f));

	l->result = floatDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Float (*Fn)();
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(), 10.2f);
} END_TEST

BEGIN_TEST(FloatFromNatTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, floatDesc(e));
	Var param = l->createParam(intDesc(e));

	*l << prolog();
	*l << ucastf(eax, param);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Float (*Fn)(Nat);
	Fn fn = (Fn)b->address();

	// Too large for a signed int:
	CHECK_EQ((*fn)(3000000000), 3000000000.0f);
} END_TEST

BEGIN_TEST(FloatFromWordTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, floatDesc(e));
	Var param = l->createParam(longDesc(e));

	*l << prolog();
	*l << ucastf(eax, param);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Float (*Fn)(Word);
	Fn fn = (Fn)b->address();

	// Too large for a signed long:
	CHECK_EQ((*fn)(100000), 100000); // Separate code-path on x86.
	CHECK_EQ((*fn)(10000000000000000000), 10000000000000000000.0f);
} END_TEST

BEGIN_TEST(FloatToNatTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, intDesc(e));
	Var param = l->createParam(floatDesc(e));

	*l << prolog();
	*l << fcastu(eax, param);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Nat (*Fn)(Float);
	Fn fn = (Fn)b->address();

	// Too large for a signed int. Number is small enough to be stored accurately in a float it seems.
	CHECK_EQ((*fn)(3000000000.0f), Nat(3000000000));
} END_TEST

BEGIN_TEST(FloatToWordTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, longDesc(e));
	Var param = l->createParam(floatDesc(e));

	*l << prolog();
	*l << fcastu(rax, param);
	*l << fnRet(rax);

	Binary *b = new (e) Binary(arena, l);
	typedef Word (*Fn)(Float);
	Fn fn = (Fn)b->address();

	// Too large for a signed long. Floats are inexact, so we make sure that the sign is handled properly.
	CHECK_GT((*fn)(10000000000000000000.0f), Word(9500000000000000000));
	CHECK_EQ((*fn)(133.0f), Word(133));
	CHECK_EQ((*fn)(-133.0f), Word(0)); // This is how the X86 implementation works, others may differ.
} END_TEST

