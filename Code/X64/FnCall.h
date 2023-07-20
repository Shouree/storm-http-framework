#pragma once
#include "../Listing.h"
#include "../TypeDesc.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		class Arena;

		/**
		 * Parameter information.
		 */
		class ParamInfo {
			STORM_VALUE;
		public:
			ParamInfo(TypeDesc *desc, const Operand &src, Bool ref);
			ParamInfo(TypeDesc *desc, const Operand &src, Bool ref, Bool lea);

			// Type of this parameter.
			TypeDesc *type;

			// Source of the parameter.
			Operand src;

			// Is 'src' a reference to the actual data?
			Bool ref;

			// Do we actually want to pass the address of 'src'?
			Bool lea;
		};


		/**
		 * Emit code required to perform a function call. Used from 'RemoveInvalid'.
		 */
		void emitFnCall(const Arena *arena, Listing *dest, Operand call, Operand resultPos, TypeDesc *resultType,
						Bool resultRef, Block currentBlock, RegSet *used, Array<ParamInfo> *params);

	}
}
