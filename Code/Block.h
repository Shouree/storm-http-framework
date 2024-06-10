#pragma once

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Represents a block of code in a listing. These blocks contain a set of local variables, along
	 * with parent-child relations. This information will be available in the final Binary object as
	 * well (but in another format).
	 */
	class Block {
		STORM_VALUE;
	public:
		// Create an invalid block.
		STORM_CTOR Block();

		inline Bool STORM_FN operator ==(Block o) const { return id == o.id; }
		inline Bool STORM_FN operator !=(Block o) const { return id != o.id; }

		// Key.
		inline Nat STORM_FN key() const { return id; }

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Output.
		void STORM_FN toS(StrBuf *to) const;

	private:
		friend class Listing;
		friend class Operand;

		friend wostream &operator <<(wostream &to, Block l);

		explicit Block(Nat id);

		Nat id;
	};

	wostream &operator <<(wostream &to, Block l);

}
