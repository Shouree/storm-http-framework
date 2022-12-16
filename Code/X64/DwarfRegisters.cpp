#include "stdafx.h"
#include "DwarfRegisters.h"
#include "Asm.h"

namespace code {
	namespace x64 {

#define REG_MAP(our, dw) if (reg == our) return dw

		Nat dwarfRegister(Reg reg) {
			reg = asSize(reg, Size::sPtr);
			REG_MAP(ptrA, DW_REG_RAX);
			REG_MAP(code::x64::ptrD, DW_REG_RDX);
			REG_MAP(ptrC, DW_REG_RCX);
			REG_MAP(ptrB, DW_REG_RBX);
			REG_MAP(code::x64::ptrSi, DW_REG_RSI);
			REG_MAP(code::x64::ptrDi, DW_REG_RDI);
			REG_MAP(ptrFrame, DW_REG_RBP);
			REG_MAP(ptrStack, DW_REG_RSP);
			REG_MAP(code::x64::ptr8, DW_REG_R8);
			REG_MAP(code::x64::ptr9, DW_REG_R9);
			REG_MAP(code::x64::ptr10, DW_REG_R10);
			REG_MAP(code::x64::ptr11, DW_REG_R11);
			REG_MAP(code::x64::ptr12, DW_REG_R12);
			REG_MAP(code::x64::ptr13, DW_REG_R13);
			REG_MAP(code::x64::ptr14, DW_REG_R14);
			REG_MAP(code::x64::ptr15, DW_REG_R15);

			assert(false, L"This register is not supported by DWARF yet.");
			return 0;
		}

	}
}
