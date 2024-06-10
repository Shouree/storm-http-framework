#pragma once
#include "Core/StrBuf.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		/**
		 * Kind of repeat for an production.
		 */
		enum RepType {
			repNone,
			repZeroOne,
			repOnePlus,
			repZeroPlus,
		};

		// Skippable?
		Bool STORM_FN skippable(RepType rep);

		// Repeatable?
		Bool STORM_FN repeatable(RepType rep);

	}
}
