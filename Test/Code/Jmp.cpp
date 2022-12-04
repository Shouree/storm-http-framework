#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"
#include "Code/UsedRegs.h"

using namespace code;

BEGIN_TEST_(JmpTest, Code) {
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

	PVAR(l);

	Binary *b = new (e) Binary(arena, l);
	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();

	CHECK_EQ((*fn)(1), 2);
	CHECK_EQ((*fn)(2), 4);
	CHECK_EQ((*fn)(3), 6);

} END_TEST

BEGIN_TEST_(UsedTest, Code) {
	// This is to test that the used registers logics works properly in code that contains jumps.
	// In this code, the registers eax and ebx are dirty throughout the loop. This would not be
	// caught by the old logic.
	Engine &e = gEngine();

	Listing *l = new (e) Listing(false, intDesc(e));

	Label t = l->label();
	Label loop = l->label();
	Label end = l->label();

	*l << prolog();

	*l << mov(eax, intConst(0));
	*l << mov(ebx, intConst(10));

	*l << loop;
	*l << cmp(eax, ebx);
	*l << jmp(t, ifLess);
	*l << jmp(end, ifEqual);

	*l << add(eax, intConst(1));
	*l << jmp(loop);

	*l << t;
	*l << add(ebx, intConst(1));
	*l << jmp(loop);

	*l << end;
	*l << fnRet(eax);

	PVAR(l);

	UsedRegs used = code::usedRegs(null, l);
	CHECK(used.used->at(8)->has(eax));
	CHECK(used.used->at(8)->has(ebx));
	CHECK(!used.used->at(1)->has(eax));
	CHECK(!used.used->at(1)->has(ebx));
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
