#pragma once
#include "../Transform.h"
#include "../OpTable.h"
#include "../UsedRegs.h"

namespace code {
	namespace x86 {
		STORM_PKG(core.asm.x86);

		/**
		 * Remove any 64-bit operands.
		 */
		class Remove64 : public Transform {
			STORM_CLASS;
		public:
			STORM_CTOR Remove64();

			// Start transform.
			virtual void STORM_FN before(Listing *dest, Listing *src);

			// Transform one instruction.
			virtual void STORM_FN during(Listing *dest, Listing *src, Nat line);

		private:
			// Remember used registers.
			Array<RegSet *> *used;

			// Signature for transform functions.
			typedef void (Remove64::*TransformFn)(Listing *dest, Instr *src, RegSet *used);

			// Transform table.
			static const OpEntry<TransformFn> transformMap[];

			// Transform functions.
			void movTfm(Listing *to, Instr *instr, RegSet *used);
			void swapTfm(Listing *to, Instr *instr, RegSet *used);
			void addTfm(Listing *to, Instr *instr, RegSet *used);
			void adcTfm(Listing *to, Instr *instr, RegSet *used);
			void borTfm(Listing *to, Instr *instr, RegSet *used);
			void bandTfm(Listing *to, Instr *instr, RegSet *used);
			void bnotTfm(Listing *to, Instr *instr, RegSet *used);
			void subTfm(Listing *to, Instr *instr, RegSet *used);
			void sbbTfm(Listing *to, Instr *instr, RegSet *used);
			void bxorTfm(Listing *to, Instr *instr, RegSet *used);
			void cmpTfm(Listing *to, Instr *instr, RegSet *used);
			void testTfm(Listing *to, Instr *instr, RegSet *used);
			void mulTfm(Listing *to, Instr *instr, RegSet *used);
			void idivTfm(Listing *to, Instr *instr, RegSet *used);
			void udivTfm(Listing *to, Instr *instr, RegSet *used);
			void imodTfm(Listing *to, Instr *instr, RegSet *used);
			void umodTfm(Listing *to, Instr *instr, RegSet *used);
			void pushTfm(Listing *to, Instr *instr, RegSet *used);
			void popTfm(Listing *to, Instr *instr, RegSet *used);
		};

	}
}
