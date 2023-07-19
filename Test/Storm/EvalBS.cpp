#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Package.h"
#include "Compiler/Function.h"

template <class T>
static T callFn(Function *f) {
	typedef T (*Ptr)();
	Ptr p = (Ptr)f->ref().address();
	return (*p)();
}

static int64 findResult(const Str *name) {
	const wchar *str = name->c_str();
	size_t len = wcslen(str);
	int64 r = 0;
	int64 pos = 1;

	for (size_t i = len; i > 0; i--) {
		wchar ch = str[i-1];
		if (ch >= '0' && ch <= '9') {
			r += (ch - '0') * pos;
			pos *= 10;
		} else if (ch == 'N') {
			r = -r;
		} else {
			break;
		}
	}

	return r;
}

static bool findBoolResult(const Str *name) {
	const wchar *str = name->c_str();
	wchar last = str[wcslen(str) - 1];
	switch (last) {
	case 'T':
		return true;
	case 'F':
		return false;
	default:
		throw TestError(L"Tests returning booleans must end with either T or F!");
	}
}

BEGIN_TEST(EvalBS, BS) {
	Engine &e = gEngine();
	Package *pkg = e.package(S("tests.eval"));
	VERIFY(pkg);

	pkg->forceLoad();
	for (NameSet::Iter i = pkg->begin(); i != pkg->end(); i++) {
		Function *fn = as<Function>(i.v());
		if (!fn)
			continue;

		if (fn->result.type == StormInfo<Bool>::type(e)) {
			bool b = findBoolResult(fn->name);
			CHECK_EQ_TITLE(callFn<Bool>(fn), b, fn->identifier());
			continue;
		}

		int64 nr = findResult(fn->name);
		if (fn->result.type == StormInfo<Byte>::type(e)) {
			CHECK_EQ_TITLE(callFn<Byte>(fn), (Byte)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Int>::type(e)) {
			CHECK_EQ_TITLE(callFn<Int>(fn), (Int)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Nat>::type(e)) {
			CHECK_EQ_TITLE(callFn<Nat>(fn), (Nat)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Long>::type(e)) {
			CHECK_EQ_TITLE(callFn<Long>(fn), (Long)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Word>::type(e)) {
			CHECK_EQ_TITLE(callFn<Word>(fn), (Word)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Float>::type(e)) {
			CHECK_EQ_TITLE(callFn<Float>(fn), (Float)nr, fn->identifier());
		} else if (fn->result.type == StormInfo<Double>::type(e)) {
			CHECK_EQ_TITLE(callFn<Double>(fn), (Double)nr, fn->identifier());
		} else {
			throw TestError(L"Unknown return type for function " + ::toS(fn->identifier()));
		}
	}
} END_TEST


BEGIN_TEST(FloatBS, BS) {
	CHECK_EQ(runFn<Float>(S("tests.bs.floatCall")), 33.0f);

	CHECK_EQ(runFn<Float>(S("tests.bs.floatParams"), 10.2f, 1.0f), 10.2f);
	CHECK_EQ(runFn<Float>(S("tests.bs.floatAdd"), 10.2f, 1.0f), 11.2f);
	CHECK_EQ(runFn<Float>(S("tests.bs.floatSub"), 10.2f, 1.0f), 9.2f);
	CHECK_EQ(runFn<Float>(S("tests.bs.floatMul"), 10.2f, 2.0f), 20.4f);
	CHECK_EQ(runFn<Float>(S("tests.bs.floatDiv"), 10.2f, 2.0f), 5.1f);
	CHECK_EQ(runFn<Float>(S("tests.bs.floatLiteral")), 11.5f);

	CHECK_EQ(runFn<Float>(S("tests.bs.floatThread")), 11.5f);
	CHECK_EQ(runFn<Float>(S("tests.bs.floatFuture")), 12.5f);

	CHECK_EQ(runFn<Int>(S("tests.bs.floatRound"), 1.6f), 1);
	CHECK_EQ(runFn<Int>(S("tests.bs.floatRound"), 1.5f), 1);
	CHECK_EQ(runFn<Int>(S("tests.bs.floatRound"), 1.4f), 1);

} END_TEST
