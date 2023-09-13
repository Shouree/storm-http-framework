#include "stdafx.h"
#include "Type.h"
#include "Exception.h"
#include "String.h"

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

	static bool isAlpha(wchar ch) {
		return (ch >= 'a' && ch <= 'z')
			|| (ch >= 'A' && ch <= 'Z');
	}

	QueryType QueryType::parse(Str *from) {
		const wchar *nameBegin = from->c_str();
		while (*nameBegin == ' ')
			nameBegin++;

		const wchar *nameEnd = nameBegin;
		while (isAlpha(*nameEnd))
			nameEnd++;

		QueryType result;
		if (compareNoCase(nameBegin, nameEnd, S("int"))) {
			result = QueryType::integer();
		} else if (compareNoCase(nameBegin, nameEnd, S("integer"))) {
			result = QueryType::integer();
		} else if (compareNoCase(nameBegin, nameEnd, S("real"))) {
			result = QueryType::real();
		} else if (compareNoCase(nameBegin, nameEnd, S("text"))) {
			result = QueryType::text();
		} else if (compareNoCase(nameBegin, nameEnd, S("varchar"))) {
			result = QueryType::text();
		} else {
			// We don't know the type. Just return the empty result.
			return result;
			// StrBuf *msg = new (from) StrBuf();
			// *msg << S("Unknown type: ");
			// *msg << new (from) Str(nameBegin, nameEnd);
			// throw new (from) SQLError(msg->toS());
		}

		// Find the size if it exists.
		const wchar *paren = nameEnd;
		while (*paren == ' ')
			paren++;

		if (*paren == '(') {
			paren++;

			Nat size = 0;
			while (*paren != ')') {
				if (*paren == ' ') {
					paren++;
					continue;
				}

				if (*paren < '0' || *paren > '9')
					throw new (from) SQLError(TO_S(from, S("Invalid size in SQL type: ") << from));

				size = size*10 + (*paren - '0');
				paren++;
			}

			// Consume end )
			paren++;

			// Consume any whitespace.
			while (*paren == ' ')
				paren++;

			result.typeSize = size + 1;
		}

		if (*paren != '\0') {
			throw new (from) SQLError(TO_S(from, S("Improperly formatted SQL type: ") << from));
		}

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
