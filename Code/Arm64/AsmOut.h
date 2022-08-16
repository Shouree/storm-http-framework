#pragma once
#include "../Listing.h"
#include "../Output.h"

namespace code {
	namespace arm64 {
		STORM_PKG(core.asm.arm64);

		// Output a listing to an Output object.
		void output(Listing *src, Output *to);

	}
}
