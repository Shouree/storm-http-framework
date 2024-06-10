#include "stdafx.h"
#include "RunOn.h"
#include "Core/Str.h"

namespace storm {

	RunOn::RunOn() : state(any), thread(null) {}

	RunOn::RunOn(State s) : state(s), thread(null) {}

	RunOn::RunOn(NamedThread *thread) : state(named), thread(thread) {}

	Bool RunOn::canRun(RunOn o) const {
		// Anyone can run something declared 'any'.
		if (o.state == any)
			return true;

		// If the other is declared as 'runtime', we can not know.
		if (o.state == runtime)
			return false;

		if (state != o.state)
			return false;
		if (state == named && thread != o.thread)
			return false;

		return true;
	}

	wostream &operator <<(wostream &to, const RunOn &v) {
		switch (v.state) {
		case RunOn::any:
			to << L"any";
			break;
		case RunOn::runtime:
			to << L"runtime";
			break;
		case RunOn::named:
			to << L"named: " << v.thread->identifier()->c_str();
			break;
		}
		return to;
	}

	void RunOn::toS(StrBuf *to) const {
		switch (state) {
		case RunOn::any:
			*to << S("any");
			break;
		case RunOn::runtime:
			*to << S("runtime");
			break;
		case RunOn::named:
			*to << S("named: ") << thread->identifier();
			break;
		}
	}

}
