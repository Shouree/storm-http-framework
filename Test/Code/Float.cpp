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
