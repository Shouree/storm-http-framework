#pragma once
#include "Core/Io/Url.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A position in a source file.
	 *
	 * Describes a range of characters somewhere in the source code. The positions in the source
	 * code are codepoint indices, not byte offset. The codepoint indices assume that line-endings
	 * have been converted into a single codepoint (exactly as would happen when reading the text
	 * using the standard library in Storm).
	 */
	class SrcPos {
		STORM_VALUE;
	public:
		// Create an unknown position.
		STORM_CTOR SrcPos();

		// Create.
		STORM_CTOR SrcPos(Url *file, Nat start, Nat end);

		// The file. May be `null` in case the file is not known.
		MAYBE(Url *) file;

		// Start position in the file.
		Nat start;

		// End position in the file.
		Nat end;

		// Is this an unknown position?
		Bool STORM_FN unknown() const;

		// Any data?
		Bool STORM_FN any() const;
		Bool STORM_FN empty() const;

		// Increase the positions.
		SrcPos STORM_FN operator +(Nat v) const;

		// Merge with another range.
		SrcPos STORM_FN extend(SrcPos other) const;

		// Get an SrcPos that represents only the first character in the range.
		SrcPos STORM_FN firstCh() const;

		// Get an SrcPos that represents only the last character in the range.
		SrcPos STORM_FN lastCh() const;

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Compare.
		Bool STORM_FN operator ==(SrcPos o) const;
		Bool STORM_FN operator !=(SrcPos o) const;

		// Output.
		void STORM_FN toS(StrBuf *to) const;
	};

	// Output.
	wostream &operator <<(wostream &to, const SrcPos &p);

}
