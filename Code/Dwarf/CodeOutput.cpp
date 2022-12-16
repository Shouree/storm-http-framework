#include "stdafx.h"
#include "CodeOutput.h"

namespace code {
	namespace dwarf {

		CodeOutput::CodeOutput() : pos(0) {}

		void CodeOutput::setFrameOffset(Offset offset) {
			fnInfo.setCFAOffset(pos, offset);
		}

		void CodeOutput::setFrameRegister(Reg reg) {
			fnInfo.setCFARegister(pos, reg);
		}

		void CodeOutput::setFrame(Reg reg, Offset offset) {
			fnInfo.setCFA(pos, reg, offset);
		}

		void CodeOutput::markSaved(Reg reg, Offset offset) {
			fnInfo.preserve(pos, reg, offset);
		}

	}
}
