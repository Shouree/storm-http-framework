#pragma once
#include "Function.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Make a small function which wraps another function so that all parameters are received as
	 * references. No changes are made to the return value.
	 */
	code::Binary *makeRefParams(Function *wrap);

	// See if 'makeRefParams' is unnecessary.
	bool allRefParams(Function *fn);

	/**
	 * Convert a 'toS()' function into a 'toS(StrBuf)' function.
	 */
	code::Binary *makeToSThunk(Function *original);

}
