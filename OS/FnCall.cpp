#include "stdafx.h"
#include "FnCall.h"

namespace os {

	FnCallRaw::FnCallRaw() {}

	FnCallRaw::FnCallRaw(void **params, CallThunk thunk) {
		this->params(params, false);
		this->thunk = thunk;
	}

#ifdef USE_MOVE
	FnCallRaw::FnCallRaw(FnCallRaw &&o) : paramsData(o.paramsData), thunk(o.thunk) {
		o.paramsData = 0;
	}

	FnCallRaw &FnCallRaw::operator =(FnCallRaw &&o) {
		if (freeParams())
			delete []params();
		this->paramsData = o.paramsData;
		this->thunk = o.thunk;
		o.paramsData = 0;
		return *this;
	}
#endif

	FnCallRaw::~FnCallRaw() {
		if (freeParams())
			delete []params();
	}

	bool FnCallRaw::freeParams() const {
		return (paramsData & 0x1) == 0x1;
	}

	void FnCallRaw::params(void **data, bool owner) {
		paramsData = size_t(data);
		assert(!freeParams(), L"'data' is misaligned!");
		if (owner)
			paramsData |= 0x1;
	}

	// Helper, used in assembler.
	extern "C" void fnCallRaw(FnCallRaw *me, const void *fn, bool member, void *first, void *result) {
		me->callRaw(fn, member, first, result);
	}

#if (defined(X86) || defined(X64)) && defined(WINDOWS)

	// Helper, used on X86 from StackCall.asm.
	extern "C" void fnCallCatch(FnCallRaw *me, const void *fn, bool member, void *first, void *result) {
		try {
			me->callRaw(fn, member, first, result);
		} catch (...) {
			PLN(L"Exceptions are not supported for stack calls on Windows.");
			terminate();
		}
	}

#endif
}
