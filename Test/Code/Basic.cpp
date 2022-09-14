#include "stdafx.h"
#include "Code/Listing.h"
#include "Code/Binary.h"
#include "Code/Exception.h"
#include "Code/X64/Layout.h"
#include "Code/Arm64/Layout.h"

using namespace code;

BEGIN_TEST(CodeScopeTest, CodeBasic) {
	Engine &e = gEngine();

	Listing *l = new (e) Listing();

	Block b1 = l->createBlock(l->root());
	Block b2 = l->createBlock(b1);
	Var v0 = l->createVar(l->root(), Size::sLong);
	Var v1 = l->createVar(b1, Size::sInt);
	Var v2 = l->createVar(b2, Size::sInt);
	Var v3 = l->createVar(b1, Size::sInt);
	Var par = l->createPtrParam();

	// *l << mov(eax, ebx);
	// *l << mov(v2, intConst(10));

	CHECK_EQ(l->prev(v0), par);
	CHECK_EQ(l->prev(v1), v0);
	CHECK_EQ(l->prev(v2), v3);
	CHECK_EQ(l->prev(v3), v1);
	CHECK_EQ(l->prev(par), Var());
	CHECK_EQ(l->parent(b1), l->root());
	CHECK_EQ(l->parent(b2), b1);

	// Make sure that the parameter is in the root block.
	CHECK_EQ(l->allVars(l->root())->count(), 2);
	CHECK_EQ(l->allVars(b1)->count(), 2);
	CHECK_EQ(l->allVars(b2)->count(), 1);

} END_TEST

BEGIN_TEST(CodeScopeTest2, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Block b0 = l->createBlock(l->root());
	Var v0 = l->createVar(b0, Size::sLong);
	Var v1 = l->createVar(b0, Size::sLong);
	Var v2 = l->createVar(b0, Size::sInt);
	Block b1 = l->createBlock(l->root());
	Var v3 = l->createVar(b1, Size::sInt);

	CHECK_EQ(l->prev(v0), Var());
	CHECK_EQ(l->prev(v1), v0);
	CHECK_EQ(l->prev(v2), v1);
	CHECK_EQ(l->prev(v3), Var());

	CHECK_EQ(l->parent(v0), b0);
	CHECK_EQ(l->parent(v1), b0);
	CHECK_EQ(l->parent(v2), b0);
	CHECK_EQ(l->parent(v3), b1);
	CHECK_EQ(l->parent(b0), l->root());
	CHECK_EQ(l->parent(b1), l->root());

	*l << prolog();
	*l << begin(b0);
	*l << end(b0);
	*l << begin(b1);
	*l << end(b1);
	*l << epilog();

	CHECK_RUNS(new (e) Binary(arena, l));

} END_TEST

BEGIN_TEST(CodeScopeTest3, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Block b1 = l->createBlock(l->root());
	Block b2 = l->createBlock(b1);

	Var v0 = l->createVar(b1, Size::sLong);

	CHECK_EQ(l->parent(v0), b1);
	CHECK_EQ(l->parent(b1), l->root());
	CHECK_EQ(l->parent(b2), b1);

	*l << prolog();
	*l << begin(b1);
	*l << lea(ptrA, v0);
	*l << end(b1);
	*l << begin(b2); // Should not be allowed!
	*l << lea(ptrA, v0);
	*l << epilog();

	CHECK_ERROR(new (e) Binary(arena, l), code::BlockBeginError);

} END_TEST

BEGIN_TEST(CodeTest, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	Block root = l->root();
	Var v = l->createIntVar(root);
	Var p = l->createIntParam();

	Label l1 = l->label();

	CHECK_EQ(l->exceptionAware(), false);

	*l << prolog();
	*l << mov(v, p);
	*l << add(v, intConst(1));
	*l << l1 << mov(ptrA, l1);
	// Use 'ebx' so that we have to preserve some registers during the function call on x64...
	*l << mov(ebx, v);
	*l << mov(eax, ebx);
	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);

	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();
	Int r = (*fn)(10);
	CHECK_EQ(r, 11);
} END_TEST


BEGIN_TEST(SwapTest, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();
	Var p = l->createIntParam();

	*l << prolog();

	*l << mov(eax, intConst(21));
	*l << swap(eax, p);
	*l << sub(eax, p);

	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(arena, l);

	typedef Int (*Fn)(Int);
	Fn fn = (Fn)b->address();
	Int r = (*fn)(100);
	CHECK_EQ(r, 79);
} END_TEST


// Do some heavy GC allocations to make the Gc do a collection.
static void triggerCollect() {
	// Note: Number of iterations reduced due to long execution time on ARM.
	for (nat j = 0; j < 2; j++) {
		for (nat i = 0; i < 100; i++)
			new (gEngine()) Str(S("Hello"));

		gEngine().gc.collect();
	}
}

// Make sure the Gc are not moving code that is referred to on the stack, even if we may have
// unaligned pointers into blocks.
BEGIN_TEST(CodeGcTest, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	*l << prolog();
	*l << call(arena->external(S("triggerCollect"), address(&triggerCollect)), Size());
	*l << mov(eax, intConst(1337));
	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(code::arena(e), l);

	typedef Int (*Fn)();
	Fn fn = (Fn)b->address();
	CHECK_EQ((*fn)(), 1337);
} END_TEST

BEGIN_TEST(CodeHereTest, CodeBasic) {
	Engine &e = gEngine();
	Arena *arena = code::arena(e);

	Listing *l = new (e) Listing();

	*l << prolog();
	*l << mov(ptrA, arena->external(S("triggerCollect"), address(&triggerCollect)));
	// *l << push(arena->external(S("triggerCollect"), address(&triggerCollect)));
	// *l << pop(ptrA);
	l->result = intDesc(e);
	*l << fnRet(eax);

	Binary *b = new (e) Binary(code::arena(e), l);

	typedef void *(*Fn)();
	Fn fn = (Fn)b->address();
	CHECK_EQ((*fn)(), address(&triggerCollect));
} END_TEST

BEGIN_TEST(CodeX64Layout, CodeBasic) {
	using namespace code::x64;

	Engine &e = gEngine();

	code::x64::Params *p = new (e) code::x64::Params();
	p->add(0, new (e) PrimitiveDesc(intPrimitive()));
	p->add(1, new (e) PrimitiveDesc(floatPrimitive()));

	SimpleDesc *s = new (e) SimpleDesc(Size::sInt*4, 4);
	s->at(0) = Primitive(primitive::integer, Size::sInt, Offset());
	s->at(1) = Primitive(primitive::real, Size::sFloat, Offset::sInt);
	s->at(2) = Primitive(primitive::real, Size::sFloat, Offset::sInt*2);
	s->at(3) = Primitive(primitive::integer, Size::sInt, Offset::sInt*3);
	p->add(2, s);

	SimpleDesc *t = new (e) SimpleDesc(Size::sInt*4, 4);
	t->at(0) = Primitive(primitive::integer, Size::sInt, Offset());
	t->at(1) = Primitive(primitive::integer, Size::sInt, Offset::sInt);
	t->at(2) = Primitive(primitive::real, Size::sFloat, Offset::sInt*2);
	t->at(3) = Primitive(primitive::real, Size::sFloat, Offset::sInt*3);
	p->add(3, t);

	SimpleDesc *u = new (e) SimpleDesc(Size::sLong*3, 4);
	t->at(0) = Primitive(primitive::integer, Size::sLong, Offset());
	t->at(1) = Primitive(primitive::integer, Size::sLong, Offset::sLong);
	t->at(2) = Primitive(primitive::integer, Size::sLong, Offset::sLong*2);
	p->add(4, u);

	CHECK_EQ(p->stackCount(), 1);
	CHECK_EQ(p->stackAt(0), 4);
	CHECK_EQ(p->registerAt(0), Param(0, 4, 0));
	CHECK_EQ(p->registerAt(6), Param(1, 4, 0));
	CHECK_EQ(p->registerAt(1), Param(2, 8, 0));
	CHECK_EQ(p->registerAt(2), Param(2, 8, 8));
	CHECK_EQ(p->registerAt(3), Param(3, 8, 0));
	CHECK_EQ(p->registerAt(7), Param(3, 8, 8));
} END_TEST

BEGIN_TEST(CodeArm64Layout, CodeBasic) {
	using namespace code::arm64;

	Engine &e = gEngine();

	code::arm64::Params *p = new (e) code::arm64::Params();
	p->add(0, new (e) PrimitiveDesc(intPrimitive()));
	p->add(1, new (e) PrimitiveDesc(floatPrimitive()));

	SimpleDesc *s = new (e) SimpleDesc(Size::sInt*4, 4);
	s->at(0) = Primitive(primitive::integer, Size::sInt, Offset());
	s->at(1) = Primitive(primitive::real, Size::sFloat, Offset::sInt);
	s->at(2) = Primitive(primitive::real, Size::sFloat, Offset::sInt*2);
	s->at(3) = Primitive(primitive::integer, Size::sInt, Offset::sInt*3);
	p->add(2, s);

	SimpleDesc *t = new (e) SimpleDesc(Size::sInt*4, 4);
	t->at(0) = Primitive(primitive::integer, Size::sInt, Offset());
	t->at(1) = Primitive(primitive::integer, Size::sInt, Offset::sInt);
	t->at(2) = Primitive(primitive::real, Size::sFloat, Offset::sInt*2);
	t->at(3) = Primitive(primitive::real, Size::sFloat, Offset::sInt*3);
	p->add(3, t);

	SimpleDesc *u = new (e) SimpleDesc(Size::sLong*3, 4);
	t->at(0) = Primitive(primitive::integer, Size::sLong, Offset());
	t->at(1) = Primitive(primitive::integer, Size::sLong, Offset::sLong);
	t->at(2) = Primitive(primitive::integer, Size::sLong, Offset::sLong*2);
	p->add(4, u);

	CHECK_EQ(p->stackCount(), 1);
	CHECK_EQ(p->stackParam(0), 4);
	CHECK_EQ(p->stackOffset(0), 0);
	CHECK_EQ(p->registerAt(0), Param(0, 4, 0, false));
	CHECK_EQ(p->registerAt(8), Param(1, 4, 0, false));
	CHECK_EQ(p->registerAt(2), Param(2, 8, 0, false));
	CHECK_EQ(p->registerAt(3), Param(2, 8, 8, false));
	CHECK_EQ(p->registerAt(4), Param(3, 8, 0, false));
	CHECK_EQ(p->registerAt(5), Param(3, 8, 8, false));
} END_TEST
