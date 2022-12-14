#pragma once
#include "Gc/DwarfRecords.h"
#include "../Reg.h"

namespace code {
	namespace dwarf {

		STORM_PKG(core.asm.dwarf);

		/**
		 * Generation of DWARF unwinding information to properly support exceptions in the generated
		 * code. Assumes that DWARF(2) unwinding data is used in the compiler. The eh_frame section
		 * (which is used during stack unwinding) contains a set of records. Each record is either a
		 * CIE record or a FDE record. CIE records describe general parameters that can be used from
		 * multiple FDE records. FDE records describe what happens in a single function.
		 *
		 * Note: Examine the file unwind-dw2-fde.h/unwind-dw2-fde.c in the libstdc++ library for
		 * details on how the exception handling is implemented on GCC. We will abuse some
		 * structures from there.
		 */

		class FDEStream;

		/**
		 * Generate information about functions, later used by the exception system on some
		 * platforms.
		 */
		class FnInfo {
			STORM_VALUE;
		public:
			// Create.
			FnInfo();

			// Set the FDE we shall write to.
			void set(FDE *fde);

			// Note that the prolog has been executed. The prolog is expected to use ptrFrame as usual.
			void prolog(Nat pos);

			// Note that the epilog has been executed.
			void epilog(Nat pos);

			// Note that a register has been stored to the stack (for preservation).
			void preserve(Nat pos, Reg reg, Offset offset);

		private:
			// The data emitted.
			FDE *target;

			// Offset inside 'to'.
			Nat offset;

			// Last position which we encoded something at.
			Nat lastPos;

			// Go to 'pos'.
			void advance(FDEStream &to, Nat pos);
		};

		// Initialize the CIE records we want.
		void initCIE(CIE *cie);

	}
}
