#include "stdafx.h"
#include "FnState.h"
#include "Code/Block.h"
#include "Code/Exception.h"

namespace code {
	namespace eh {

		Nat encodeFnState(Nat block, Nat activation) {
			if (block != Block().key() && block > 0xFFFE)
				throw new (runtime::someEngine()) InvalidValue(S("The X86 backend does not support more than 65535 blocks."));
			if (activation > 0xFFFF)
				throw new (runtime::someEngine()) InvalidValue(S("The X86 backend does not support more than 65536 activations."));

			if (block == Block().key())
				block = 0xFFFF;

			return (block << 16) | activation;
		}

		void decodeFnState(Nat original, Nat &block, Nat &activation) {
			block = (original >> 16) & 0xFFFF;
			activation = original & 0xFFFF;

			if (block == 0xFFFF)
				block = Block().key();
		}

	}
}
