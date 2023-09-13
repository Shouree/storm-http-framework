#include "stdafx.h"
#include "String.h"

namespace sql {

	static bool eq(const wchar a, const wchar b) {
		if (a == b)
			return true;
		if (b >= 'A' && b <= 'Z')
			if (a == b + ('a' - 'A'))
				return true;
		if (b >= 'a' && b <= 'z')
			if (a == b - ('a' - 'A'))
				return true;
		return false;
	}

	bool compareNoCase(Str *compare, const wchar *to) {
		const wchar *at;
		for (at = compare->c_str(); *at; at++, to++) {
			if (!eq(*at, *to))
				return false;
		}

		return *at == 0 && *to == 0;
	}

	bool compareNoCase(const wchar *compareBegin, const wchar *compareEnd, const wchar *to) {
		const wchar *at;
		for (at = compareBegin; at < compareEnd; at++, to++) {
			if (!eq(*at, *to))
				return false;
		}

		return at == compareEnd && *to == 0;
	}

	Str *toSQLStrLiteral(Str *x) {
		StrBuf *out = new (x) StrBuf();
		*out << S("'");

		for (Str::Iter i = x->begin(); i != x->end(); ++i) {
			if (i.v() == Char('\''))
				*out << S("''");
			else
				*out << i.v();
		}

		*out << S("'");
		return out->toS();
	}
}
