#include "stdafx.h"
#include "InfoErrors.h"
#include "InfoNode.h"

namespace storm {
	namespace syntax {

		static const Nat failedMask  = 0x80000000;
		static const Nat shiftsMask  = 0x7FFF0000;
		static const Nat charsMask   = 0x0000FFFF;
		static const Nat shiftsShift = 16;

		InfoErrors::InfoErrors() : data(0) {}

		InfoErrors::InfoErrors(Nat data) : data(data) {}

		Bool InfoErrors::success() const {
			return (data & failedMask) == 0;
		}

		Bool InfoErrors::any() const {
			// Checks: success() == false || shifts() > 0 || chars() > 0
			return data != 0;
		}

		Nat InfoErrors::shifts() const {
			return (data & shiftsMask) >> shiftsShift;
		}

		Nat InfoErrors::chars() const {
			return data & charsMask;
		}

		void InfoErrors::chars(Nat chars) {
			data &= ~charsMask;
			data |= min(chars, charsMask);
		}

		InfoErrors InfoErrors::operator +(InfoErrors e) const {
			Nat f = (data & failedMask) | (e.data & failedMask);
			Nat a = min(shifts() + e.shifts(), shiftsMask >> shiftsShift);
			Nat b = min(chars() + e.chars(), charsMask);

			return InfoErrors(f | (a << shiftsShift) | b);
		}

		InfoErrors &InfoErrors::operator +=(InfoErrors e) {
			// We're just an int, so this is fairly reasonable.
			*this = *this + e;
			return *this;
		}

		static inline Nat value(InfoErrors e) {
			if (!e.success())
				return failedMask;
			else
				return e.shifts() + e.chars();
		}

		Bool InfoErrors::operator <(InfoErrors e) const {
			return value(*this) < value(e);
		}

		Bool InfoErrors::operator ==(InfoErrors e) const {
			return value(*this) == value(e);
		}

		Bool InfoErrors::operator !=(InfoErrors e) const {
			return value(*this) != value(e);
		}

		InfoErrors infoFailure() {
			return InfoErrors(failedMask);
		}

		InfoErrors infoSuccess() {
			return InfoErrors(0);
		}

		InfoErrors infoShifts(Nat shifts) {
			shifts = min(shifts, shiftsMask >> shiftsShift);
			return InfoErrors((shifts << shiftsShift) & shiftsMask);
		}

		InfoErrors infoChars(Nat chars) {
			chars = min(chars, charsMask);
			return InfoErrors(chars & charsMask);
		}

		Nat InfoErrors::getData(InfoErrors e) {
			return e.data;
		}

		InfoErrors InfoErrors::fromData(Nat v) {
			return InfoErrors(v);
		}

		void InfoErrors::toS(StrBuf *to) const {
			if (success()) {
				if (!any()) {
					*to << S("success");
				} else {
					*to << chars() << (chars() == 1 ? S(" char") : S(" chars")) << L", ";
					*to << shifts() << (shifts() == 1 ? S(" correction") : S(" corrections"));
				}
			} else {
				*to << S("failure");
			}
		}

		wostream &operator <<(wostream &to, InfoErrors e) {
			if (e.success()) {
				if (!e.any()) {
					to << L"success";
				} else {
					to << e.chars() << (e.chars() == 1 ? L" char" : L" chars") << L", ";
					to << e.shifts() << (e.shifts() == 1 ? L" correction" : L" corrections");
				}
			} else {
				to << L"failure";
			}

			return to;
		}

	}
}
