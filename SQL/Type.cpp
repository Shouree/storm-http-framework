#include "stdafx.h"
#include "Type.h"
#include "Exception.h"

namespace sql {

	QueryType::QueryType() : type(0), typeSize(0) {}

	QueryType::QueryType(Nat typeId) : type(typeId), typeSize(0) {}

	QueryType::QueryType(Nat typeId, Nat size) : type(typeId), typeSize(size) {}

	Bool QueryType::compatible(const QueryType &o) const {
		if (!sameType(o))
			return false;

		if (typeSize == 0)
			return true;

		// TODO: Maybe allow "larger than" also?
		return typeSize == o.typeSize;
	}

	// Note: Assumes 'to' is small ASCII characters!
	static bool compare(Str *cmp, const wchar *to) {
		for (const wchar *i = cmp->c_str(); *i; i++) {
			if (*i == *to || *i == *to - ('a' - 'A')) {
				to++;
			} else if (*to == 0) {
				// We're at the end. Ensure that 'i' only contains spaces, and possibly a start paren.
				if (*i != ' ' && *i != '(')
					return false;
			} else {
				return false;
			}
		}

		return *to == 0;
	}

	QueryType QueryType::parse(Str *from) {
		Nat size = 0;
		QueryType result;

		Str::Iter paren = from->find(Char('('));
		if (paren != from->end()) {
			Str::Iter endParen = from->find(Char(')'), paren);
			Str *sz = from->substr(paren + 1, endParen);
			if (!sz->isNat())
				throw new (from) SQLError(TO_S(from, S("Incorrect size in type: ") << from));
			size = sz->toNat() + 1;
		}

		if (compare(from, S("int")))
			result = QueryType::integer();
		else if (compare(from, S("integer")))
			result = QueryType::integer();
		else if (compare(from, S("real")))
			result = QueryType::real();
		else if (compare(from, S("text")))
			result = QueryType::text();
		else if (compare(from, S("varchar")))
			result = QueryType::text();

		if (result.sameType(QueryType()))
			throw new (from) SQLError(TO_S(from, S("Unknown type: ") << from));

		result.typeSize = size;
		return result;
	}

	StrBuf &operator <<(StrBuf &to, const QueryType &t) {
		if (t.sameType(QueryType())) {
			to << S("VOID");
		} else if (t.sameType(QueryType::text())) {
			to << S("TEXT");
		} else if (t.sameType(QueryType::integer())) {
			to << S("INTEGER");
		} else if (t.sameType(QueryType::real())) {
			to << S("REAL");
		} else {
			to << S("<invalid type>");
		}

		Maybe<Nat> size = t.size();
		if (size.any())
			to << S("(") << size.value() << S(")");

		return to;
	}

}
