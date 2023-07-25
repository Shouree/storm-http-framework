#pragma once
#include "../Arena.h"
#include "../Reg.h"
#include "../Operand.h"

namespace code {
	namespace x86 {
		STORM_PKG(core.asm.x86);

		/**
		 * Arena for X86 (TODO: factor windows/unix versions of this one!)
		 */
		class Arena : public code::Arena {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Arena();

			/**
			 * Transform.
			 */

			virtual Listing *STORM_FN transform(Listing *src) const;
			virtual void STORM_FN output(Listing *src, Output *to) const;

			/**
			 * Outputs.
			 */

			virtual LabelOutput *STORM_FN labelOutput() const;
			virtual CodeOutput *STORM_FN codeOutput(Binary *owner, LabelOutput *size) const;

			/**
			 * Registers.
			 */

			virtual void STORM_FN removeFnRegs(RegSet *from) const;

			/**
			 * Misc.
			 */

			virtual Listing *STORM_FN redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param);
			virtual Listing *STORM_FN engineRedirect(TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand engine);
			virtual Nat STORM_FN firstParamId(MAYBE(TypeDesc *) desc);
			virtual Operand STORM_FN firstParamLoc(Nat id);
			virtual Reg STORM_FN functionDispatchReg();

		};


	}
}
