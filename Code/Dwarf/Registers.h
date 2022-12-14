#pragma once
#include "../Reg.h"

namespace code {
	namespace dwarf {

		Nat dwarfRegister(Reg reg);

		/**
		 * Register numbers in the DWARF world. Not the same as the numbers used in instruction
		 * encodings, sadly.
		 */
#define DW_REG_RAX 0
#define DW_REG_RDX 1
#define DW_REG_RCX 2
#define DW_REG_RBX 3
#define DW_REG_RSI 4
#define DW_REG_RDI 5
#define DW_REG_RBP 6
#define DW_REG_RSP 7
#define DW_REG_R8 8
#define DW_REG_R9 9
#define DW_REG_R10 10
#define DW_REG_R11 11
#define DW_REG_R12 12
#define DW_REG_R13 13
#define DW_REG_R14 14
#define DW_REG_R15 15
#define DW_REG_RA 16 // return address (virtual)


		/**
		 * Generic register numbers.
		 * Note: on X86-64 these correspond to EAX and EDX
		 * Note: on Arm64 these correspond to r0 and r1
		 */
#define DWARF_EH_RETURN_0 __builtin_eh_return_data_regno(0)
#define DWARF_EH_RETURN_1 __builtin_eh_return_data_regno(1)
#define DWARF_EH_SP __builtin_dwarf_sp_column()

	}
}
