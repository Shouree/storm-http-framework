#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../Reg.h"
#include "../Instr.h"
#include "Core/Array.h"

namespace code {
	class Binary;

	namespace arm64 {
		STORM_PKG(core.asm.arm64);

		/**
		 * Transforms all accesses to local variables into sp/bp relative addresses. Also generates
		 * function prolog/epilog as well as any construction/destruction required for the blocks in
		 * the listing.
		 */
		class Layout : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR Layout(Binary *owner);

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

			// After done. Adds metadata.
			virtual void STORM_FN after(Listing *dest, Listing *src);

		private:
			// Owner.
			Binary *owner;

			// Variable layout.
			Array<Offset> *layout;
		};

		// Compute the layout of variables, given a listing, parameters and the number of registers
		// that need to be spilled into memory in the function prolog and epilog.
		Array<Offset> *STORM_FN layout(Listing *l, /* Params *params, */ Nat spilled);

	}
}
