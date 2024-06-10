#pragma once
#include "Size.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A variable inside a listing.
	 */
	class Var {
		STORM_VALUE;
	public:
		// Create an invalid value.
		STORM_CTOR Var();

		inline Bool STORM_FN operator ==(Var o) const { return id == o.id; }
		inline Bool STORM_FN operator !=(Var o) const { return id != o.id; }

		// Get the size of our variable.
		inline Size STORM_FN size() const { return sz; }

		// Get our id. Mainly for use in backends.
		inline Nat STORM_FN key() const { return id; }

		// Output.
		void STORM_FN toS(StrBuf *to) const;

	private:
		friend class Listing;
		friend class Operand;
		friend wostream &operator <<(wostream &to, Var v);

		Var(Nat id, Size size);

		Nat id;
		Size sz;
	};

	wostream &operator <<(wostream &to, Var v);
}
