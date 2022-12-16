#include "stdafx.h"
#include "FunctionInfo.h"
#include "Utils/Bitwise.h"
#include "Registers.h"
#include "Stream.h"
#include "Code/PosixEh/Personality.h"

namespace code {
	namespace dwarf {


		/**
		 * Initialize CIE records to match the output from FnInfo.
		 */

		Nat initStormCIE(CIE *cie, Nat codeAlign, Int dataAlign, Nat returnColumn) {
			cie->id = 0;
			cie->version = 1;
			DStream out(cie->data, CIE_DATA);
			out.putByte('z'); // There is a size of all augmentation data.
			out.putByte('R'); // FDE encoding of addresses.
			out.putByte('P'); // Personality function.
			out.putByte(0);   // End of the string.

			out.putUNum(codeAlign); // code alignment
			out.putSNum(dataAlign); // data alignment factor
			out.putUNum(returnColumn); // location of the return address

			out.putUNum(2 + sizeof(void *)); // size of augmentation data
			out.putByte(DW_EH_PE_absptr); // absolute addresses
			out.putByte(DW_EH_PE_absptr); // encoding of the personality function
			out.putPtr(address(&code::eh::stormPersonality));

			assert(!out.overflow(), L"Increase CIE_DATA!");
			return out.pos;
		}


		/**
		 * Emit function information.
		 */

		FunctionInfo::FunctionInfo() : target(null), codeAlignment(0), dataAlignment(0), offset(0), lastPos(0) {}

		void FunctionInfo::set(FDE *fde, Nat code, Int data, Bool is64, RegToDwarf toDwarf) {
			this->target = fde;
			this->codeAlignment = code;
			this->dataAlignment = data;
			this->is64 = is64;
			this->offset = fde->firstFree();
			this->lastPos = 0;
			this->regToDwarf = toDwarf;
		}

		void FunctionInfo::setCFAOffset(Nat pos, Offset offset) {
			FDEStream to(target, this->offset);
			advance(to, pos);
			to.putUOp(DW_CFA_def_cfa_offset, getOffset(offset));
		}

		void FunctionInfo::setCFARegister(Nat pos, Reg reg) {
			FDEStream to(target, this->offset);
			advance(to, pos);
			to.putUOp(DW_CFA_def_cfa_register, (*regToDwarf)(reg));
		}

		void FunctionInfo::setCFA(Nat pos, Reg reg, Offset offset) {
			FDEStream to(target, this->offset);
			advance(to, pos);
			to.putUOp(DW_CFA_def_cfa, (*regToDwarf)(reg), getOffset(offset));
		}

		void FunctionInfo::preserve(Nat pos, Reg reg, Offset offset) {
			FDEStream to(target, this->offset);
			advance(to, pos);

			// Note that we stored the variable.
			// Note: These are factored according to the data alignment factor, which we set to -8 above.
			Int off = getOffset(offset) / dataAlignment;
			assert(off >= 0, L"Offset should be positive, the backend is broken.");
			to.putUOp(DW_CFA_offset + (*regToDwarf)(reg), off);
		}

		void FunctionInfo::advance(FDEStream &to, Nat pos) {
			assert(pos >= lastPos);
			if (pos > lastPos) {
				assert((pos - lastPos) % codeAlignment == 0);
				to.putAdvance((pos - lastPos) / codeAlignment);
				lastPos = pos;
			}
		}

		Int FunctionInfo::getOffset(Offset offset) {
			if (is64)
				return offset.v64();
			else
				return offset.v32();
		}

	}
}
