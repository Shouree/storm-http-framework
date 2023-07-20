#pragma once
#include "../Arena.h"
#include "Params.h"

namespace code {
	namespace x64 {
		STORM_PKG(core.asm.x64);

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
			virtual CodeOutput *STORM_FN codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const;

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
			virtual code::Params *createParams() const ABSTRACT;

			// Layout parameters.
			code::Params *layoutParams(TypeDesc *result, Array<TypeDesc *> *params);
		};


		class WindowsArena : public Arena {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WindowsArena();

			/**
			 * Parameters.
			 */
			virtual Nat STORM_FN firstParamId(MAYBE(TypeDesc *) desc);
			virtual Operand STORM_FN firstParamLoc(Nat id);
			virtual code::Params *createParams() const {
				return new (this) WindowsParams();
			}
		};


		class PosixArena : public Arena {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR PosixArena();

			/**
			 * Transform.
			 */


			/**
			 * Parameters.
			 */
			virtual Nat STORM_FN firstParamId(MAYBE(TypeDesc *) desc);
			virtual Operand STORM_FN firstParamLoc(Nat id);
			virtual code::Params *createParams() const {
				return new (this) PosixParams();
			}
		};

	}
}
