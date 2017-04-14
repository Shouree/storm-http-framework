#pragma once
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class InfoNode;

		/**
		 * Description of the error corrections performed to arrive at an info tree.
		 */
		class InfoErrors {
			STORM_VALUE;
		public:
			// Create. Assumess success and zero errors.
			STORM_CTOR InfoErrors();

			// Was this parse successful at all?
			Bool STORM_FN success() const;

			// Any errors in here? (succes() == false yields error() == true)
			Bool STORM_FN error() const;

			// Get # of shifts during error corrections.
			Nat STORM_FN shifts() const;

			// Get # of skipped characters during error corrections.
			Nat STORM_FN skipped() const;

			// Add two errors together.
			InfoErrors STORM_FN operator +(InfoErrors e) const;
			InfoErrors &STORM_FN operator +=(InfoErrors e);

			// Compare <, compares the sum of the two errors.
			Bool STORM_FN operator <(InfoErrors e) const;

			// Equal an not equal comparisions based on the < operator.
			Bool STORM_FN operator ==(InfoErrors e) const;
			Bool STORM_FN operator !=(InfoErrors e) const;

			// Get/set integer representation.
			static Nat getData(InfoErrors e);
			static InfoErrors fromData(Nat v);

		private:
			// Low-level create.
			InfoErrors(Nat data);

			// Data.
			// bit 31    - set on complete failure.
			// bit 30-16 - # of productions that were shifted or reduced abnormally
			// bit 15- 0 - # of characters that were skipped
			Nat data;

			// Friends.
			friend InfoErrors infoFailure();
			friend InfoErrors infoSuccess();
			friend InfoErrors infoShifts(Nat shifts);
			friend InfoErrors infoSkipped(Nat skipped);
		};

		// Create. Indicate failure.
		InfoErrors STORM_FN infoFailure();

		// Create. Indicate success.
		InfoErrors STORM_FN infoSuccess();

		// Create. Indicate # of productions skipped.
		InfoErrors STORM_FN infoShifts(Nat shifts);

		// Create. Indicate # of chars skipped.
		InfoErrors STORM_FN infoSkipped(Nat skipped);

		// Output.
		StrBuf &STORM_FN operator <<(StrBuf &to, InfoErrors e);

	}
}
