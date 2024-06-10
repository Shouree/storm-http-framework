#pragma once
#include "Core/Object.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Kind of indentation for a production.
		 */
		enum IndentType {
			indentNone,

			// Increase indentation one level.
			indentIncrease,

			// Decrease indentation one level.
			indentDecrease,

			// Increase indentation one level as long as no other considered nodes affected
			// indentation (except for align directives).
			indentWeakIncrease,

			// Align with the start of the token before the indented range. Only the leafmost
			// alignment is considered when indentation information is generated.
			indentAlignBegin,

			// Align with the end of the token before the indented range. Only the leafmost
			// alignment is considered when indentation information is generated.
			indentAlignEnd,
		};

		// Output.
		StrBuf &STORM_FN operator <<(StrBuf &to, IndentType i);


		/**
		 * Information about indentation in an InfoNode. This is only needed in the minority of
		 * nodes, so it is only created when it is actually needed.
		 */
		class InfoIndent : public Object {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR InfoIndent(Nat start, Nat end, IndentType type);

			// Start and end position of the intended range.
			Nat start;
			Nat end;

			// Type of indentation.
			IndentType type;

			// Is 'i' included in the range?
			Bool STORM_FN contains(Nat i) const;

			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


		/**
		 * Represents possible results from an indent-query. Either: 'n levels of indentation' or
		 * 'same indentation as line on offset n'.
		 */
		class TextIndent {
			STORM_VALUE;
		public:
			// Create, zero levels of indentation.
			STORM_CTOR TextIndent();

			// Is this an alignment indentation?
			Bool STORM_FN isAlign() const;

			// Access the indentation level. Returns zero if we're containing a character offset.
			Int STORM_FN level() const;
			void STORM_FN level(Int level);

			// Access the current offset. Returns zero if we're not containing a character offset.
			Nat STORM_FN alignAs() const;
			void STORM_FN alignAs(Nat offset);

			// Have we seen some indentation directive?
			Bool STORM_FN seenIndent() const;

			// Apply indentation of a parent node, assuming it is applicable to us.
			// Provided is the offset of the start of the repetition for the parent node,
			// and the current token id.
			void STORM_FN applyParent(InfoIndent *info, Nat current, Nat repOffsetBegin, Nat repOffsetEnd);

			// Increase any offsets in this structure by 'n'.
			void STORM_FN offset(Nat n);

			// Compare.
			inline Bool STORM_FN operator ==(TextIndent o) const { return (value & ~indentMask) == (o.value & ~indentMask); }
			inline Bool STORM_FN operator !=(TextIndent o) const { return !(*this == o); }

			// Output.
			void STORM_FN toS(StrBuf *to) const;

		private:
			// Store the result as follows:
			// topmost bit indicates that we have seen a indentIncrease or indentDecrease.
			// second topmost is set if we contain an offset into the file.
			// the rest of the bits either contains a 2:s complement of the indentation level
			// or an unsigned number representing the offset.
			Nat value;

			// Masks.
			static const Nat indentMask   = 0x80000000;
			static const Nat relativeMask = 0x40000000;
			static const Nat signMask     = 0x20000000;
			static const Nat dataMask     = 0x1FFFFFFF;
		};

		// Output.
		wostream &operator <<(wostream &to, TextIndent i);

		// Create initialized TextIndent instances.
		TextIndent STORM_FN indentLevel(Int level);
		TextIndent STORM_FN indentAs(Nat offset);

	}
}
