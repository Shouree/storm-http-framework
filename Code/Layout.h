#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Listing.h"
#include "Size.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Compute the layout of all local variables inside a listing. Does not even attempt to generate
	 * offsets for parameters. Those entries are left as Offset() (ie. zero).
	 *
	 * The last entry in the returned array is the size of the stack, aligned to sPtr.
	 */
	Array<Offset> *STORM_FN layout(Listing *src);

	// Like "layout", but allows specifying a minimum alignment for individual variables.
	Array<Offset> *STORM_FN layout(Listing *src, Size minAlign);

}
