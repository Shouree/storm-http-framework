#include "stdafx.h"
#include "Label.h"
#include "Core/StrBuf.h"

namespace code {

	Label::Label() : id(-1) {}

	Label::Label(Nat id) : id(id) {}

	void Label::deepCopy(CloneEnv *) {}

	wostream &operator <<(wostream &to, Label l) {
		return to << l.id;
	}

	StrBuf &operator <<(StrBuf &to, Label l) {
		return to << l.id;
	}

}
