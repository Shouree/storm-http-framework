#pragma once
#include "../Arena.h"
#include "WindowsParams.h"
#include "PosixParams.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

		class Layout;

		/**
		 * Arena for X86-64. Use WindowsArena or PosixArena for different flavours of the 64-bit
		 * arena. The main difference between the two systems is the calling convention.
		 */
		class Arena : public code::Arena {
			STORM_ABSTRACT_CLASS;
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

			/**
			 * Registers.
			 */

			virtual void STORM_FN removeFnRegs(RegSet *from) const;

			/**
			 * Misc.
			 */

			virtual Listing *STORM_FN redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param);
			virtual Listing *STORM_FN engineRedirect(TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand engine);
			virtual Reg STORM_FN functionDispatchReg();


			/**
			 * Parameter layout (new interface here):
			 */

			// Create parameter layout.
			virtual code::Params *STORM_FN createParams() const ABSTRACT;

			// Layout parameters.
			code::Params *layoutParams(TypeDesc *result, Array<TypeDesc *> *params);

			// Create the layout transform.
			virtual Layout *STORM_FN layoutTfm() const ABSTRACT;

			// Registers that are not preserved across function calls. Initialized by subclasses.
			RegSet *dirtyRegs;
		};


		class WindowsArena : public Arena {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WindowsArena();

			/**
			 * Output.
			 */
			virtual LabelOutput *STORM_FN labelOutput() const;
			virtual CodeOutput *STORM_FN codeOutput(Binary *owner, LabelOutput *size) const;

			/**
			 * Parameters.
			 */
			virtual Nat STORM_FN firstParamId(MAYBE(TypeDesc *) desc);
			virtual Operand STORM_FN firstParamLoc(Nat id);
			virtual code::Params *STORM_FN createParams() const;
			virtual Layout *STORM_FN layoutTfm() const;

		};


		class PosixArena : public Arena {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR PosixArena();

			/**
			 * Output.
			 */
			virtual CodeOutput *STORM_FN codeOutput(Binary *owner, LabelOutput *size) const;

			/**
			 * Parameters.
			 */
			virtual Nat STORM_FN firstParamId(MAYBE(TypeDesc *) desc);
			virtual Operand STORM_FN firstParamLoc(Nat id);
			virtual code::Params *STORM_FN createParams() const;
			virtual Layout *STORM_FN layoutTfm() const;

		};

	}
}
