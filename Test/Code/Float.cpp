#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(FloatTest, Code) {
	TODO(L"ENABLE");
	break;

	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p1 = l->createFloatParam();
	Var p2 = l->createFloatParam();
	Var v1 = l->createIntVar(l->root());

	*l << prolog();

	*l << fld(p1);
	*l << fld(p2);
	*l << fmulp();
	*l << mov(v1, intConst(10));
	*l << fild(v1);
	*l << fmulp();
	*l << fistp(v1);
	*l << mov(eax, v1);

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
