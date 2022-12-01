#include "stdafx.h"
#include "Code/Binary.h"
#include "Code/Listing.h"

#include "Code/Arm64/Asm.h"

using namespace code;

static Nat bitmask(Nat n, Nat imms, Nat immr) {
	return n << 12 | immr << 6 | imms;
}

BEGIN_TEST(Arm64BitmaskGenerator, Code) {
	// Some basic tests of the bitmask generator.
	CHECK_EQ(code::arm64::encodeBitmask(0x00FFFF00, false), bitmask(0, 0x0F, 0x8));
	CHECK_EQ(code::arm64::encodeBitmask(0xEEEEEEEE, false), bitmask(0, 0x3A, 0x1));
	CHECK_EQ(code::arm64::encodeBitmask(0x0000FFFFFFFF0000, true), bitmask(1, 0x1F, 0x10));
	CHECK_EQ(code::arm64::encodeBitmask(0xEEEEEEEEEEEEEEEE, true), bitmask(0, 0x3A, 0x1));
	CHECK_EQ(code::arm64::encodeBitmask(0xE1E1E1E1E1E1E1E1, true), bitmask(0, 0x33, 0x5));
	CHECK_EQ(code::arm64::encodeBitmask(0x0000000000100000, true), bitmask(1, 0x00, 0x14));

	// These are not possible to encode:
	CHECK_EQ(code::arm64::encodeBitmask(0x0010001000001000, true), 0);
	CHECK_EQ(code::arm64::encodeBitmask(0x0000000000000000, true), 0);
	CHECK_EQ(code::arm64::encodeBitmask(0xFFFFFFFFFFFFFFFF, true), 0);
} END_TEST


// Generates code that uses bitmasks on the ARM platform.
static Nat useBitmask32(Word bitmask) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, intDesc(e));

	*l << prolog();

	*l << mov(eax, intConst(0));

	// On ARM, the or operation is encoded using the bitmask method.
	*l << bor(eax, intConst(bitmask));

	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);
	typedef Nat (*Fn)();
	Fn fn = (Fn)b->address();
	return (*fn)();
}

static Word useBitmask64(Word bitmask) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing(false, longDesc(e));

	*l << prolog();

	*l << mov(rax, wordConst(0));

	// On ARM, the or operation is encoded using the bitmask method.
	*l << bor(rax, wordConst(bitmask));

	*l << fnRet(rax);

	Binary *b = new (e) Binary(arena, l);
	typedef Word (*Fn)();
	Fn fn = (Fn)b->address();
	return (*fn)();
}

BEGIN_TEST(Arm64BitmaskTest, Code) {
	CHECK_EQ(useBitmask32(0x0FF00FF0), 0x0FF00FF0);
	CHECK_EQ(useBitmask32(0x00FFFF00), 0x00FFFF00);
	CHECK_EQ(useBitmask32(0xEEEEEEEE), 0xEEEEEEEE);

	CHECK_EQ(useBitmask64(0x000FFFFFF0000000), 0x000FFFFFF0000000);
	CHECK_EQ(useBitmask64(0xEEEEEEEEEEEEEEEE), 0xEEEEEEEEEEEEEEEE);
	CHECK_EQ(useBitmask64(0xE1E1E1E1E1E1E1E1), 0xE1E1E1E1E1E1E1E1);
	CHECK_EQ(useBitmask64(0x0000000000001000), 0x0000000000001000);

	// These will have to use some fallback.
	CHECK_EQ(useBitmask64(0x0010000010001001), 0x0010000010001001);

	// These might be a special case in the implementation.
	CHECK_EQ(useBitmask64(0x0000000000000000), 0x0000000000000000);
	CHECK_EQ(useBitmask64(0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF);
} END_TEST
