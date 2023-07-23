#pragma once
#include "Layout.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Layout transform specialized for the Windows calling convention.
		 */
		class WindowsLayout : public Layout {
			STORM_CLASS;
		public:
			STORM_CTOR WindowsLayout(const Arena *arena);

		protected:
			// Compute layout.
			virtual Array<Offset> *STORM_FN computeLayout(Listing *l, Params *params, Nat spilled);

			// Offset of result parameter.
			virtual Offset STORM_FN resultParam();

			// Save/restore result.
			virtual void STORM_FN saveResult(Listing *dest);
			virtual void STORM_FN restoreResult(Listing *dest);
		};

	}
}
