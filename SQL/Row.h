#pragma once
#include "Core/GcArray.h"
#include "Core/GcType.h"
#include "Core/Variant.h"

namespace sql {

	/**
	 * Representation of a single row as a result from a database query.
	 *
	 * Behaves similarly to a read-only array, but that may contain different types. The accessor
	 * functions that mention specific types throw if the type is incorrect. Type inspection is
	 * possible through the generic 'at' function that creates a Variant (note: the at function is a
	 * free function for technical reasons).
	 */
	class Row {
		STORM_VALUE;
	public:
		// For ability to use in a Maybe<T> in C++.
		Row();

		// Deep copy to maintain thread safety.
		void STORM_FN deepCopy(CloneEnv *env);

		// Get number of columns.
		Nat STORM_FN count() const { return Nat(data->count); }

		// Get a column as a string.
		Str *STORM_FN getStr(Nat index) const;

		// Get a column as a boolean.
		Bool STORM_FN getBool(Nat index) const;

		// Get a column as an integer.
		Int STORM_FN getInt(Nat index) const;

		// Get a column as a long.
		Long STORM_FN getLong(Nat index) const;

		// Get a column as a float.
		Float STORM_FN getFloat(Nat index) const;

		// Get a column as a double.
		Double STORM_FN getDouble(Nat index) const;

		// Check if a column is null.
		Bool STORM_FN isNull(Nat index) const;

	private:
		// Primitive types. Stored in the object value as pointers.
		enum ColType {
			COL_NULL = 0, // null pointer for empty
			COL_LONG,     // long
			COL_DOUBLE,   // double

			COL_LAST,     // last one, for quick check that the type is an object.
		};

		struct Element {
			// Pointer to an object. Also used to indicate the type of primitives.
			// Note: We sometimes store small integers here. Inspecting the MPS code
			// reveals that this should be safe, since the MPS only examines pointers
			// that refer to memory managed by the MPS. The page from 0 to approx 4k
			// are never managed by the MPS, so this is safe to do.
			void *object;

			// Data for primitive objects.
			union {
				Long l;
				Double d;
			};
		};

		// Type for the elements.
		static const GcType elemType;

		// Data storage. Read-only, so assumed to be immutable.
		GcArray<Element> *data;

		// Check if index is valid.
		const Element &checkIndex(Nat index) const;

		// Throw type error.
		void throwTypeError(const wchar *expected, const Element &elem) const;

		// At function.
		friend Variant at(EnginePtr e, const Row &row, Nat index);

	public:

		/**
		 * Builder class for a row.
		 */
		class Builder {
			STORM_VALUE;
		public:
			// Create.
			Builder(GcArray<Element> *elems);

			// Add columns.
			void STORM_FN push(Str *v);
			void STORM_FN push(Long l);
			void STORM_FN push(Double d);
			void STORM_FN pushNull();

		private:
			// Data being populated.
			GcArray<Element> *data;

			// Check so that we are not full.
			Element &next();

			friend class Row;
		};

		// Create from a builder.
		STORM_CTOR Row(Builder b);

		// Create a builder.
		static Builder STORM_FN builder(EnginePtr e, Nat columns);
	};

	// Get a column. Creates a Variant, so is slower than using typed accessors.
	Variant STORM_FN at(EnginePtr e, const Row &row, Nat index);

	// Print a row, for convenience.
	StrBuf *STORM_FN operator <<(StrBuf *to, const Row &row);

}
