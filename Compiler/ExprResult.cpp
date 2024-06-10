#include "stdafx.h"
#include "ExprResult.h"
#include "Core/StrBuf.h"

namespace storm {

	ExprResult::ExprResult() : returns(true) {}

	ExprResult::ExprResult(Value result) : result(result), returns(true) {}

	ExprResult::ExprResult(Bool any) : returns(any) {}

	Value ExprResult::type() const {
		if (returns)
			return result;
		else
			return Value();
	}

	Bool ExprResult::value() const {
		return returns && result != Value();
	}

	Bool ExprResult::empty() const {
		return returns && result == Value();
	}

	Bool ExprResult::nothing() const {
		return !returns;
	}

	Bool ExprResult::operator ==(const ExprResult &o) const {
		return returns == o.returns && result == o.result;
	}

	Bool ExprResult::operator !=(const ExprResult &o) const {
		return !(*this == o);
	}

	ExprResult ExprResult::asRef(Bool r) const {
		if (nothing())
			return *this;
		return ExprResult(result.asRef(r));
	}

	wostream &operator <<(wostream &to, const ExprResult &r) {
		if (r.nothing()) {
			to << L"<nothing>";
		} else {
			to << r.type();
		}
		return to;
	}

	void ExprResult::toS(StrBuf *to) const {
		if (nothing()) {
			*to << S("nothing");
		} else {
			*to << type();
		}
	}

	void ExprResult::deepCopy(CloneEnv *env) {
		result.deepCopy(env);
	}

	ExprResult noReturn() {
		return ExprResult(false);
	}

	ExprResult common(ExprResult a, ExprResult b) {
		if (a.nothing())
			return b;
		if (b.nothing())
			return a;

		return common(a.type(), b.type());
	}

}
