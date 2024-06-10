#include "stdafx.h"
#include "Var.h"
#include "Core/StrBuf.h"

namespace code {

	Var::Var() : id(-1) {}

	Var::Var(Nat id, Size size) : id(id), sz(size) {}

	wostream &operator <<(wostream &to, Var v) {
		if (v == Var())
			return to << L"invalid var";
		return to << L"Var" << v.id << L":" << v.sz;
	}

	void Var::toS(StrBuf *to) const {
		if (*this == Var()) {
			*to << S("invalid var");
		} else {
			*to << S("Var") << id << S(":") << sz;
		}
	}

}
