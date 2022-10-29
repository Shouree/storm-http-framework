#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

using namespace code;

BEGIN_TEST(JmpTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();

	*l << prolog();

	Label loop = l->label();
	Label end = l->label();

	*l << mov(eax, intConst(0));

	*l << loop;
	*l << cmp(p, intConst(0));
	*l << jmp(end, ifEqual);

	*l << sub(p, intConst(1));
	*l << add(eax, intConst(2));
	*l << jmp(loop);

	*l << end;

	*l << epilog();
	*l << ret(Size::sInt);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(1), 2);
	CHECK_EQ((*fn)(2), 4);
	CHECK_EQ((*fn)(3), 6);

} END_TEST

static bool called = false;

static Int addFive() {
	called = true;
	return 512;
}

// Main reason for this test is to see that we properly compute the offset for static things. If we
// fail, things will fail miserably later on!
BEGIN_TEST(CallTest, Code) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	*l << prolog();

	*l << call(arena->external(S("addFive"), address(&addFive)), Size::sInt);

	*l << epilog();
	*l << ret(Size::sInt);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();

	called = false;
	CHECK_EQ((*fn)(), 512);
	CHECK(called);

} END_TEST
