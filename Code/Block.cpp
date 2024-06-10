#include "stdafx.h"
#include "Block.h"
#include "Core/StrBuf.h"

namespace code {

	Block::Block() : id(-1) {}

	Block::Block(Nat id) : id(id) {}

	void Block::deepCopy(CloneEnv *) {}

	wostream &operator <<(wostream &to, Block l) {
		if (l.id == Block().id)
			return to << L"invalid block";
		return to << L"Block" << l.id;
	}

	void Block::toS(StrBuf *to) const {
		if (id == Block().id) {
			*to << S("invalid block");
		} else {
			*to << S("Block") << id;
		}
	}

}
