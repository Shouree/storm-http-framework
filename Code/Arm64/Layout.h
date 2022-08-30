#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../Reg.h"
#include "../Instr.h"
#include "Core/Array.h"
#include "Params.h"

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

			// Parameters.
			Params *params;

			// Result.
			Result *result;

			// Current block.
			Block currentBlock;

			// Registers that need to be preserved.
			RegSet *preserved;

			// Offset where "pointer to result" is stored, if we need it.
			Operand resultLocation() {
				return ptrRel(ptrFrame, Offset(16));
			}

			// Resolve local variables.
			Operand resolve(Listing *src, const Operand &op);
			Operand resolve(Listing *src, const Operand &op, const Size &size);

			// Transform table.
			typedef void (Layout::*TransformFn)(Listing *dest, Instr *src);
			static const OpEntry<TransformFn> transformMap[];

			// Transform of specific instructions.
			void prologTfm(Listing *dest, Instr *src);
			void epilogTfm(Listing *dest, Instr *src);
			void beginBlockTfm(Listing *dest, Instr *src);
			void endBlockTfm(Listing *dest, Instr *src);
			void jmpBlockTfm(Listing *dest, Instr *src);
			void activateTfm(Listing *dest, Instr *src);
			void fnRetTfm(Listing *dest, Instr *src);
			void fnRetRefTfm(Listing *dest, Instr *src);
		};

		// Compute the layout of variables, given a listing, parameters and the number of registers
		// that need to be spilled into memory in the function prolog and epilog.
		Array<Offset> *STORM_FN layout(Listing *l, Params *params, Nat spilled);

	}
}
