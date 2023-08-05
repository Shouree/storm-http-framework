#include "stdafx.h"
#include "Fn.h"
#include "Compiler/Exception.h"

BEGIN_TEST(StrConcatTest, BS) {
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.strConcatTest"))), L"ab1cvoid");
	CHECK_ERROR(runFn<Str *>(S("tests.bs.strConcatError")), SyntaxError);

	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.strInterpolate"))), L"|30|   20|20   |   20|+++30|00000028|");

	// Note: We probably want to use 2 digits after the "e" always. But for this, we need to provide
	// our own implementation...
#if defined(VISUAL_STUDIO) && VISUAL_STUDIO < 2013
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.strInterpolateFloat"))), L"|2.3|1e+003|2.35|1000.20|2.35e+000|1.00e+003|2.3456|1000.2|");
#else
	CHECK_EQ(::toS(runFn<Str *>(S("tests.bs.strInterpolateFloat"))), L"|2.3|1e+03|2.35|1000.20|2.35e+00|1.00e+03|2.3456|1000.2|");
#endif
} END_TEST


BEGIN_TEST(RemoveIndentTest, BS) {
	Engine &e = gEngine();

	Str *v = new (e) Str(S("  hello\n  world"));
	CHECK_EQ(::toS(removeIndentation(v)), L"hello\nworld");

	v = new (e) Str(S("hello\n  world"));
	CHECK_EQ(::toS(removeIndentation(v)), L"hello\n  world");

	v = new (e) Str(S("\n  hello\n  world"));
	CHECK_EQ(::toS(removeIndentation(v)), L"\nhello\nworld");

	v = new (e) Str(S("\nhello\n  world"));
	CHECK_EQ(::toS(removeIndentation(v)), L"\nhello\n  world");

	v = new (e) Str(S("  hello\n  \n  world"));
	CHECK_EQ(::toS(removeIndentation(v)), L"hello\n\nworld");

} END_TEST

BEGIN_TEST(TrimBlankLinesTest, BS) {
	Engine &e = gEngine();

	CHECK_EQ(::toS(trimBlankLines(new (e) Str(S("hello\n\nworld")))), L"hello\n\nworld");

	CHECK_EQ(::toS(trimBlankLines(new (e) Str(S("\n\nhello\n\nworld")))), L"hello\n\nworld");

	CHECK_EQ(::toS(trimBlankLines(new (e) Str(S("hello\n\nworld\n")))), L"hello\n\nworld");

	CHECK_EQ(::toS(trimBlankLines(new (e) Str(S("hello\n\nworld\n\n")))), L"hello\n\nworld");

	CHECK_EQ(::toS(trimBlankLines(new (e) Str(S("\n\nhello\n\nworld\n\n")))), L"hello\n\nworld");

} END_TEST

static const wchar *utfString = S("a\u00D6\u0D36\u3042\u79C1\U0001F639e");
static nat codepoints[] = {
	nat('a'), 0xD6, 0x0D36, 0x3042, 0x79C1, 0x0001F639, nat('e')
};

BEGIN_TEST(StrIteratorTest, BS) {
	Engine &e = gEngine();

	Str *v = new (e) Str(utfString);
	nat at = 0;
	for (Str::Iter i = v->begin(); i != v->end(); i++, at++) {
		int cp = 0;
		if (at < ARRAY_COUNT(codepoints))
			cp = codepoints[at];
		CHECK_EQ(i.v().codepoint(), cp);
	}

	CHECK_EQ(at, ARRAY_COUNT(codepoints));


	CHECK_EQ(runFn<Int>(S("tests.bs.iterateStr"), v), ARRAY_COUNT(codepoints));

	Array<Char> *ref = new (e) Array<Char>();
	for (nat i = 0; i < ARRAY_COUNT(codepoints); i++) {
		ref->push(Char(codepoints[i]));
	}

	CHECK_EQ(runFn<Int>(S("tests.bs.verifyStr"), v, ref), 1);

} END_TEST
