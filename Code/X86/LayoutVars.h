#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../UsedRegs.h"

namespace code {
	class Binary;

	namespace x86 {
		STORM_PKG(core.asm.x86);

		/**
		 * Transform all accesses to local variables into accesses relative to ebp. In the process,
		 * also generates function prolog and epilog, as well as any construction/destruction
		 * required for the blocks in here.
		 *
		 * Note: Make sure not to add any extra used registers here, as this may cause the prolog
		 * and/or epilog to fail preserving some registers.
		 *
		 * Note: This should be the last transform run on a listing, because of the above notice.
		 */
		class LayoutVars : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR LayoutVars();

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat id);

			// When done. Adds metadata.
			virtual void STORM_FN after(Listing *dest, Listing *src);

		private:
			// Registers saved in the prolog.
			RegSet *preserved;

			// Are we using an exception handler?
			Bool usingEH;

			// Is there a hidden parameter for the return value?
			Bool resultParam;

			// Is this a member function?
			Bool memberFn;

			// Array of activation id:s at each label. We use these to update the activation ID
			// where necessary at each jump. If an element is "INACTIVE", then that instruction is
			// an "activate" or a "begin" instruction, meaning that we don't need to do anything
			// special.
			Array<Nat> *lblActivation;

			// Layout of variables.
			Array<Offset> *layout;

			// Index where each variable was activated.
			Array<Nat> *activated;

			// Current activation ID.
			Nat activationId;

			// Current block.
			Block block;

			// Offset at which the block id is stored if we're using exceptions.
			Offset blockId;

			// Label to the beginning of the code, so that we can store a pointer to ourselves in the EH frame.
			Label selfLbl;

			// Create an Operand referring to the return pointer.
			Operand resultLoc();

			// Signature for transform functions.
			typedef void (LayoutVars::*TransformFn)(Listing *dest, Listing *src, Nat line);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Transform functions.
			void prologTfm(Listing *dest, Listing *src, Nat line);
			void epilogTfm(Listing *dest, Listing *src, Nat line);
			void beginBlockTfm(Listing *dest, Listing *src, Nat line);
			void endBlockTfm(Listing *dest, Listing *src, Nat line);
			void jmpBlockTfm(Listing *dest, Listing *src, Nat line);
			void activateTfm(Listing *dest, Listing *src, Nat line);
			void fnRetTfm(Listing *dest, Listing *src, Nat line);
			void fnRetRefTfm(Listing *dest, Listing *src, Nat line);
			void jmpTfm(Listing *dest, Listing *src, Nat line);

			// Lookup variables to their corresponding offsets.
			Operand resolve(Listing *src, const Operand &op);

			// Initialize a block or a block.
			void initBlock(Listing *dest, Block p, Reg freeReg);

			// Destroy a block.
			void destroyBlock(Listing *dest, Block p, bool preserveEax);

			// Update the block-id variable if needed.
			void updateBlockId(Listing *dest);
			void updateBlockId(Listing *dest, Nat activation);
		};


		/**
		 * Variable layouts. Extends code::layout by also providing offsets for parameters, and
		 * aligns the naive offsets returned from there to what actually happens on an X86 using
		 * cdecl calling convention.
		 *
		 * 'savedRegs' - # of saved registers in this case.
		 * 'usingEh' - using exception handler (on Windows)?
		 */
		Array<Offset> *STORM_FN layout(Listing *src, Nat savedRegs, Bool usingEh, Bool resultParam, Bool member);

	}
}
