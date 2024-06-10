#include "stdafx.h"
#include "KeyChord.h"

namespace gui {

	void KeyChord::toS(StrBuf *to) const {
		if (modifiers)
			*to << name(to->engine(), modifiers) << S("+");
		*to << c_name(key);
	}

}
