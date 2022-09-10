#pragma once
#include "Block.h"
#include "Label.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Generic data structure used in various back-ends to keep track of active blocks during code
	 * generation.
	 *
	 * Used by the X64 and Arm64 backends among others.
	 */
	class ActiveBlock {
		STORM_VALUE;
	public:
		ActiveBlock(Block block, Nat activated, Label pos);

		// Which block?
		Block block;

		// Which activation ID?
		Nat activated;

		// Where does the block start?
		Label pos;
	};

}
