#pragma once
#include "Code/Output.h"
#include "FunctionInfo.h"

namespace code {
	namespace dwarf {
		STORM_PKG(core.asm.dwarf);

		/**
		 * Code output that provides a FunctionInfo object as well.
		 */
		class CodeOutput : public code::CodeOutput {
			STORM_CLASS;
		public:
			STORM_CTOR CodeOutput();

			virtual void STORM_FN setFrameOffset(Offset offset);
			virtual void STORM_FN setFrameRegister(Reg reg);
			virtual void STORM_FN setFrame(Reg reg, Offset offset);
			virtual void STORM_FN markSaved(Reg reg, Offset offset);

		protected:
			// Function information to update. Expected to be initialized by a subclass.
			FunctionInfo fnInfo;

			// Current position in the code. Expected to be updated by a subclass.
			Nat pos;
		};

	}
}
