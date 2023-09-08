#include "stdafx.h"
#include "Exception.h"

namespace sql {

	SQLError::SQLError(Str *msg) : msg(msg), code() {}

	SQLError::SQLError(Str *msg, Maybe<Nat> code) : msg(msg), code(code) {}

	void SQLError::message(StrBuf *to) const {
		*to << S("SQL error: ") << msg;
		if (code.any())
			*to << S(" (") << code.value() << S(")");
	}

}
