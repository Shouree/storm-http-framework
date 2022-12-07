#pragma once
#include "Params.h"
#include "Asm.h"
#include "../Transform.h"
#include "../OpTable.h"
#include "../UsedRegs.h"
#include "../ActiveBlock.h"

namespace code {
	class Binary;

	namespace x64 {
		STORM_PKG(core.asm.x64);

		/**
		 * Transform all accesses to local variables into ebp-relative addresses. In the process,
		 * also generates function prolog and epilog as well as any construction/destruction
		 * required for the blocks in the listing.
		 *
		 * Note: Make sure not to add any extra used registers here, as this may cause the prolog
		 * and/or epilog to fail preserving some registers.
		 *
		 * Note: This should be the last transform run on a listing because of the above.
		 */
		class Layout : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR Layout(Binary *owner);

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

			// When done. Adds metadata.
			virtual void STORM_FN after(Listing *dest, Listing *src);

		private:
			// Owner. We need to store it when using exceptions.
			Binary *owner;

			// Layout of all parameters for this function.
			Params *params;

			// Layout of the result of this function.
			Result *result;

			// Registers that need to be preserved in the function prolog.
			RegSet *toPreserve;

			// Layout of the stack. The stack offset of all variables in the listings.
			Array<Offset> *layout;

			// Index where each variable was activated.
			Array<Nat> *activated;

			// Current activation ID.
			Nat activationId;

			// Currently active block.
			Block block;

			// Temporary storage of active blocks.
			Array<ActiveBlock> *activeBlocks;

			// Using exception handling here?
			Bool usingEH;

			// Offset of the result parameter (if any).
			Offset resultParam();

			// Signature of the transform functions.
			typedef void (Layout::*TransformFn)(Listing *dest, Listing *src, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Transform functions.
			void prologTfm(Listing *dest, Listing *src, Nat line);
			void epilogTfm(Listing *dest, Listing *src, Nat line);
			void beginBlockTfm(Listing *dest, Listing *src, Nat line);
			void endBlockTfm(Listing *dest, Listing *src, Nat line);
			void jmpBlockTfm(Listing *dest, Listing *src, Nat line);
			void activateTfm(Listing *dest, Listing *src, Nat line);

			// Function returns.
			void fnRetTfm(Listing *dest, Listing *src, Nat line);
			void fnRetRefTfm(Listing *dest, Listing *src, Nat line);

			// Alter a single operand. Replace any local variables with their offset.
			Operand resolve(Listing *src, const Operand &op);
			Operand resolve(Listing *src, const Operand &op, const Size &size);

			// Create and destroy blocks.
			void initBlock(Listing *dest, Block init, Reg reg);
			void destroyBlock(Listing *dest, Block destroy, Bool preserveRax, Bool notifyTable);

			// Spill parameters to the stack.
			void spillParams(Listing *dest);
		};


		// Compute the layout of variables, given a listing, parameters and the number of registers
		// that need to be spilled into memory in the function prolog and epilog.
		Array<Offset> *STORM_FN layout(Listing *l, Params *params, Nat spilled);

	}
}
