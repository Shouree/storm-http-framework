#include "stdafx.h"
#include "ActiveBlock.h"

namespace code {

	ActiveBlock::ActiveBlock(Block block, Nat activated, Label pos) : block(block), activated(activated), pos(pos) {}

}
