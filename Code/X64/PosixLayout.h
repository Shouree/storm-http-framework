#pragma once
#include "Layout.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Layout transform specialized for the Posix calling convention.
		 */
		class PosixLayout : public Layout {
			STORM_CLASS;
		public:
			STORM_CTOR PosixLayout(const Arena *arena);

		protected:
			// Compute layout.
			virtual Array<Offset> *STORM_FN computeLayout(Listing *l, Params *params, Nat spilled);

			// Location of the result parameter, if any.
			virtual Offset STORM_FN resultParam();

			// Save/restore result.
			virtual void STORM_FN saveResult(Listing *dest);
			virtual void STORM_FN restoreResult(Listing *dest);

			// Prolog/epilog.
			virtual void STORM_FN emitProlog(Listing *dest);
			virtual void STORM_FN emitEpilog(Listing *dest);
		};

	}
}
