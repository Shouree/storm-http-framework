#include "stdafx.h"
#include "Core/Array.h"
#include "Core/Str.h"

#include "Core/Random.h"
#include "Core/Timing.h"

BEGIN_TEST(ArrayTest, Core) {
	Engine &e = gEngine();

	Array<Str *> *t = new (e) Array<Str *>();
	CHECK_EQ(toS(t), L"[]");

	t->push(new (e) Str(L"Hello"));
	t->push(new (e) Str(L"World"));
	CHECK_EQ(toS(t), L"[Hello, World]");

	t->insert(0, new (e) Str(L"Well"));
	CHECK_EQ(toS(t), L"[Well, Hello, World]");

	t->append(t);
	CHECK_EQ(toS(t), L"[Well, Hello, World, Well, Hello, World]");

} END_TEST

BEGIN_TEST(ArrayExTest, CoreEx) {
	Engine &e = gEngine();

	// Check automatically generated toS functions.

	Array<Value> *v = new (e) Array<Value>();
	CHECK_EQ(toS(v), L"[]");

	v->push(Value(Str::stormType(e)));
	CHECK_EQ(toS(v), L"[core.Str]");

} END_TEST

static bool CODECALL predicate(Int a, Int b) {
	a = (a + 5) % 10;
	b = (b + 5) % 10;
	return a < b;
}

BEGIN_TEST(ArraySortTest, CoreEx) {
	Engine &e = gEngine();

	Array<Int> *v = new (e) Array<Int>();
	for (Int i = 0; i < 10; i++)
		*v << (10 - i);

	v->sort();
	CHECK_EQ(toS(v), L"[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]");

	// Try with a predicate!
	v->sort(fnPtr(e, &predicate));
	CHECK_EQ(toS(v), L"[5, 6, 7, 8, 9, 10, 1, 2, 3, 4]");

	// See if 'sorted' works as expected.
	CHECK_EQ(toS(v->sorted()), L"[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]");
	CHECK_EQ(toS(v), L"[5, 6, 7, 8, 9, 10, 1, 2, 3, 4]");
} END_TEST

BEGIN_TEST(ArrayRemoveDupTest, CoreEx) {
	Engine &e = gEngine();

	Array<Int> *v = new (e) Array<Int>();
	v->push(1);
	v->push(2);
	v->push(2);
	v->push(3);
	v->push(3);
	v->push(3);
	v->push(4);

	Array<Int> *w = v->withoutDuplicates();
	CHECK_EQ(toS(w), L"[1, 2, 3, 4]");
	CHECK_EQ(toS(v), L"[1, 2, 2, 3, 3, 3, 4]");

	v->removeDuplicates();
	CHECK_EQ(toS(v), L"[1, 2, 3, 4]");
} END_TEST

BEGIN_TEST(ArrayBinSearchTest, CoreEx) {
	Engine &e = gEngine();

	Array<Int> *v = new (e) Array<Int>();
	v->push(1);
	v->push(2);
	v->push(2);
	v->push(3);
	v->push(3);
	v->push(3);
	v->push(4);
	v->push(4);

	CHECK_EQ(v->lowerBound(3), 3);
	CHECK_EQ(v->upperBound(3), 6);
	CHECK_EQ(v->lowerBound(0), 0);
	CHECK_EQ(v->upperBound(0), 0);
	CHECK_EQ(v->lowerBound(4), 6);
	CHECK_EQ(v->upperBound(4), 8);
	CHECK_EQ(v->lowerBound(10), 8);
	CHECK_EQ(v->upperBound(10), 8);
} END_TEST

BEGIN_TEST(ArrayCompareTest, CoreEx) {
	Engine &e = gEngine();

	Array<Int> *a = new (e) Array<Int>();
	a->push(10);
	a->push(20);
	a->push(30);

	Array<Int> *b = new (e) Array<Int>();
	b->push(10);
	b->push(30);

	Array<Int> *c = new (e) Array<Int>();
	c->push(10);
	c->push(30);

	CHECK(a->lessRaw(b));
	CHECK(!b->lessRaw(a));

	CHECK(!a->equalRaw(b));
	CHECK(b->equalRaw(c));
} END_TEST

// Performance of sort()
BEGIN_TESTX(ArraySortPerf, CoreEx) {
	Engine &e = gEngine();

	Array<Int> *v = new (e) Array<Int>();
	vector<Int> s;
	for (Int i = 0; i < 100000; i++) {
		Int val = rand(0, 1000000);
		*v << val;
		s.push_back(val);
	}

	Moment start;
	v->sort();
	Moment half;
	std::sort(s.begin(), s.end());
	Moment end;

	PLN(L"Sorted " << v->count() << L" integers:");
	PLN(L"  " << (half - start) << L" using storm::sort");
	PLN(L"  " << (end - half)   << L" using std::sort");
} END_TEST
