#pragma once
#include "Asm.h"
#include "Code/Params.h"
#include "Code/Transform.h"
#include "Code/OpTable.h"
#include "Code/UsedRegs.h"
#include "Code/ActiveBlock.h"

namespace code {
	class Binary;

	namespace x64 {
		STORM_PKG(core.asm.x64);

		class Arena;

		/**
		 * Transform all accesses to local variables into ebp-relative addresses. In the process,
		 * also generates function prolog and epilog as well as any construction/destruction
		 * required for the blocks in the listing.
		 *
		 * Note: Make sure not to add any extra used registers during this transform, as this may
		 * cause the prolog and/or epilog to fail preserving some registers.
		 *
		 * Note: This should be the last transform run on a listing because of the above.
		 */
		class Layout : public Transform {
			STORM_ABSTRACT_CLASS;
		public:
			STORM_CTOR Layout(const Arena *arena);

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

			// When done. Adds metadata.
			virtual void STORM_FN after(Listing *dest, Listing *src);

		protected:
			/**
			 * Interface that derived classes extend.
			 */

			// Layout variables, parameters, and spilled registers. The exact layout depends on the
			// calling convention, so this is implemented in WindowsLayout and PosixLayout
			// respectively.
			virtual Array<Offset> *STORM_FN computeLayout(Listing *l, Params *params, Nat spilled) ABSTRACT;

			// Offset of the result parameter (if any).
			virtual Offset STORM_FN resultParam() ABSTRACT;

			// Save/restore result while emitting 'endblock' statements.
			virtual void STORM_FN saveResult(Listing *dest) ABSTRACT;
			virtual void STORM_FN restoreResult(Listing *dest) ABSTRACT;

		protected:
			/**
			 * Internal functionality, but accessible to derived classes.
			 */

			// Arena, for platform-specific concerns.
			const Arena *arena;

			// Layout of all parameters for this function.
			Params *params;

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
			void initBlock(Listing *dest, Block init, Operand space);
			void destroyBlock(Listing *dest, Block destroy, Bool preserveRax, Bool notifyTable);
			void epilog(Listing *dest, Listing *src, Nat line, Bool preserveRax);

			// Spill parameters to the stack.
			void spillParams(Listing *dest);
		};


	}
}
