#pragma once

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A label inside a listing.
	 */
	class Label {
		STORM_VALUE;
	public:
		STORM_CTOR Label();

		inline Bool STORM_FN operator ==(Label o) const { return id == o.id; }
		inline Bool STORM_FN operator !=(Label o) const { return id != o.id; }

	private:
		friend class Listing;
		friend class Output;
		friend class Operand;
		friend class Binary;
		friend wostream &operator <<(wostream &to, Label l);
		friend StrBuf &operator <<(StrBuf &to, Label l);

		explicit Label(Nat id);

		Nat id;
	};

	wostream &operator <<(wostream &to, Label l);
	StrBuf &operator <<(StrBuf &to, Label l);

}
