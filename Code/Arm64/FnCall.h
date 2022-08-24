#pragma once
#include "../Listing.h"
#include "../TypeDesc.h"

namespace code {
	namespace arm64 {
		STORM_PKG(core.asm.arm64);

		/**
		 * Parameter information.
		 */
		class ParamInfo {
			STORM_VALUE;
		public:
			ParamInfo(TypeDesc *desc, const Operand &src, Bool ref);

			// Type of this parameter.
			TypeDesc *type;

			// Source of the parameter.
			Operand src;

			// Is 'src' a reference to the actual data?
			Bool ref;

			// Should we pass the address of 'src'? Used internally.
			Bool lea;
		};

		/**
		 * Emit code required to perform a function call. Used from 'RemoveInvalid'.
		 */
		void emitFnCall(Listing *dest, Operand call, Operand resultPos, TypeDesc *resultType,
						Bool resultRef, Block currentBlock, RegSet *used, Array<ParamInfo> *params);

	}
}
