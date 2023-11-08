#pragma once
#include "Core/Maybe.h"

namespace sql {

	/**
	 * A description of a datatype in SQL.
	 *
	 * This type aims to be generic in the sense that it aims to represent the type in a
	 * representation that is usable across databases.
	 *
	 * A type as described here contains a type name (as an ID), and an optional size of the type.
	 */
	class QueryType {
		STORM_VALUE;
	public:
		// Create a type representing "void".
		STORM_CTOR QueryType();

		// Create the type TEXT.
		static QueryType STORM_FN text() { return QueryType(1); }

		// Create the type INTEGER.
		static QueryType STORM_FN integer() { return QueryType(2); }

		// Create the type REAL.
		static QueryType STORM_FN real() { return QueryType(3); }

		// Parse from string. Returns "void" if the type is unknown. Throws `SQLError` on malformed
		// type specification.
		static QueryType STORM_FN parse(Str *from);

		// Create a copy of the type with the specified size.
		QueryType STORM_FN sized(Nat size) const {
			return QueryType(type, size + 1);
		}

		// Get the size.
		Maybe<Nat> STORM_FN size() const {
			if (typeSize == 0)
				return Maybe<Nat>();
			else
				return Maybe<Nat>(typeSize - 1);
		}

		// Check if the types are the same, ignoring the size.
		Bool STORM_FN sameType(const QueryType &o) const {
			return type == o.type;
		}

		// Check if we are compatible with the parameter. Compatibility compares the size, but
		// allows any size of `o`, assuming our size is not specified.
		Bool STORM_FN compatible(const QueryType &o) const;

		// Is this type empty (i.e. `void`).
		Bool STORM_FN empty() const { return type == 0; }

		// Is there any type (i.e. not `void`).
		Bool STORM_FN any() const { return type != 0; }

	private:
		// Create a custom type.
		QueryType(Nat typeId);
		QueryType(Nat typeId, Nat size);

		// Type id.
		Nat type;

		// Type size - 1. Zero means no size.
		Nat typeSize;

		friend class QueryStr;
		friend class QueryStrBuilder;
	};

	// Output.
	StrBuf &STORM_FN operator <<(StrBuf &to, const QueryType &t);

}
