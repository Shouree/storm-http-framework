#include "stdafx.h"
#include "InfoIndent.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {

		/**
		 * IndentType.
		 */

		StrBuf &STORM_FN operator <<(StrBuf &to, IndentType i) {
			switch (i) {
			case indentNone:
				to << L"none";
				break;
			case indentIncrease:
				to << L"+";
				break;
			case indentDecrease:
				to << L"-";
				break;
			case indentWeakIncrease:
				to << L"?";
				break;
			case indentAlignBegin:
				to << L"@";
				break;
			case indentAlignEnd:
				to << L"$";
				break;
			}
			return to;
		}


		/**
		 * InfoIndent.
		 */

		InfoIndent::InfoIndent(Nat start, Nat end, IndentType type) : start(start), end(end), type(type) {}

		Bool InfoIndent::contains(Nat i) const {
			return i >= start && i < end;
		}

		void InfoIndent::toS(StrBuf *to) const {
			*to << L"[" << start << L"-" << end << L"]" << type;
		}


		/**
		 * TextIndent.
		 */

		TextIndent::TextIndent() : value(0) {}

		Bool TextIndent::isAlign() const {
			return (value & relativeMask) != 0;
		}

		Int TextIndent::level() const {
			if (!isAlign()) {
				// Sign-extend to the last bit.
				Nat r = value & dataMask;
				if (value & signMask)
					return r | relativeMask | indentMask;
				else
					return r;
			} else {
				return 0;
			}
		}

		void TextIndent::level(Int v) {
			value = (Nat(v) & dataMask) | indentMask;
		}

		Nat TextIndent::alignAs() const {
			if (isAlign())
				return value & dataMask;
			else
				return 0;
		}

		void TextIndent::alignAs(Nat offset) {
			value = offset | relativeMask | (value & indentMask);
		}

		Bool TextIndent::seenIndent() const {
			return (value & indentMask) != 0;
		}

		void TextIndent::applyParent(InfoIndent *info, Nat current, Nat offsetBegin, Nat offsetEnd) {
			Bool inside = info->contains(current);

			switch (info->type) {
			case indentIncrease:
				if (!isAlign())
					level(level() + inside);
				break;
			case indentDecrease:
				if (!isAlign())
					level(level() - inside);
				break;
			case indentWeakIncrease:
				if (!isAlign() && !seenIndent() && inside) {
					level(1);
				}
				break;
			case indentAlignBegin:
				if (isAlign()) {
					// We only care about the leafmost one.
				} else if (inside) {
					alignAs(offsetBegin);
				}
				break;
			case indentAlignEnd:
				if (isAlign()) {
					// We only care about the leafmost one.
				} else if (inside) {
					alignAs(offsetEnd);
				}
				break;
			}
		}

		void TextIndent::offset(Nat n) {
			if (isAlign())
				alignAs(alignAs() + n);
		}

		void TextIndent::toS(StrBuf *to) const {
			if (isAlign())
				*to << S("align as ") << alignAs();
			else
				*to << S("indent ") << level() << S(" levels");
		}

		wostream &operator <<(wostream &to, TextIndent i) {
			if (i.isAlign())
				to << L"align as " << i.alignAs();
			else
				to << L"indent " << i.level() << L" levels";
			return to;
		}

		TextIndent indentLevel(Int level) {
			TextIndent i;
			i.level(level);
			return i;
		}

		TextIndent indentAs(Nat offset) {
			TextIndent i;
			i.alignAs(offset);
			return i;
		}

	}
}
