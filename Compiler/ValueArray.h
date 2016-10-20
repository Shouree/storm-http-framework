#pragma once
#include "Core/Object.h"
#include "Core/GcArray.h"
#include "Core/Array.h" // For ArrayError.
#include "Value.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Special array type for values. This is needed as we can not use Array<Value> to instantiate
	 * template classes of Array. Instead, that part of the compiler uses this class as an array
	 * instead.
	 */
	class ValueArray : public Object {
		STORM_CLASS;
	public:
		// Empty array.
		STORM_CTOR ValueArray();

		// Copy another array.
		STORM_CTOR ValueArray(ValueArray *o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Size.
		inline Nat STORM_FN count() const { return data ? data->filled : 0; }

		// Clear.
		inline void STORM_FN clear();

		// Any elements?
		inline Bool STORM_FN any() const { return count() > 0; }

		// Empty?
		inline Bool STORM_FN empty() const { return count() == 0; }

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Access.
		Value &STORM_FN operator [](Nat id) {
			if (id > count())
				throw ArrayError(L"Index " + ::toS(id) + L" out of bounds.");
			return data->v[id];
		}

		Value &at(Nat id) {
			return operator[](id);
		}

		// Push elements.
		void STORM_FN push(Value v);

		ValueArray &operator <<(Value v) { push(v); return *this; }

		// TODO: Iterator!
	private:
		// Data.
		GcArray<Value> *data;

		// Array type.
		const GcType *arrayType;

		// Ensure 'data' can hold at least 'n' objects.
		void ensure(Nat n);
	};

}
