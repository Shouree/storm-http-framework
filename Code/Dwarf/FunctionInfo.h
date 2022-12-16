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

		typedef Nat (*RegToDwarf)(Reg);

		/**
		 * Generate information about functions, later used by the exception system on some
		 * platforms.
		 */
		class FunctionInfo {
			STORM_VALUE;
		public:
			// Create.
			FunctionInfo();

			// Set the FDE we shall write to, and specify the data alignment of the CIE record.
			void set(FDE *fde, Nat codeAlignment, Int dataAlignment, Bool is64, RegToDwarf toDwarf);

			// Define CFA offset.
			void setCFAOffset(Nat pos, Offset offset);

			// Define CFA register.
			void setCFARegister(Nat pos, Reg reg);

			// Define CFA offset *and* register.
			void setCFA(Nat pos, Reg reg, Offset offset);

			// Note that a register has been stored to the stack (for preservation).
			void preserve(Nat pos, Reg reg, Offset offset);

		private:
			// The data emitted.
			FDE *target;

			// Convert register numbers.
			UNKNOWN(PTR_NOGC) RegToDwarf regToDwarf;

			// Code alignment factor.
			Nat codeAlignment;

			// Data alignment factor.
			Int dataAlignment;

			// Use 64-bit offsets?
			Bool is64;

			// Offset inside 'to'.
			Nat offset;

			// Last position which we encoded something at.
			Nat lastPos;

			// Go to 'pos'.
			void advance(FDEStream &to, Nat pos);

			// Get correct offset.
			Int getOffset(Offset offset);
		};

		// Initialize CIE records for Storm.

		// Initialize the CIE records that correspond to what Storm uses. Data alignment is specific
		// for backends, so it is provided as a parameter. Returns the current position in the CIE
		// record if some backend wishes to continue writing.
		// 'returnColumn' is the column where the return address is stored.
		Nat initStormCIE(CIE *cie, Nat codeAlignment, Int dataAlignment, Nat returnColumn);

	}
}
