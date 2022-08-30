#include "stdafx.h"
#include "Arena.h"
#include "Reg.h"
#include "X86/Arena.h"
#include "X64/Arena.h"
#include "Arm64/Arena.h"
#include "Core/Str.h"

namespace code {

	Arena::Arena() {}

	Ref Arena::external(const wchar *name, const void *ptr) const {
		return Ref(externalSource(name, ptr));
	}

	RefSource *Arena::externalSource(const wchar *name, const void *ptr) const {
		RefSource *src = new (this) StrRefSource(name);
		src->setPtr(ptr);
		return src;
	}

	Listing *Arena::transform(Listing *src, Binary *owner) const {
		assert(false);
		return src;
	}

	void Arena::output(Listing *src, Output *to) const {
		assert(false);
	}

	LabelOutput *Arena::labelOutput() const {
		assert(false);
		return null;
	}

	CodeOutput *Arena::codeOutput(Binary *owner, Array<Nat> *offsets, Nat size, Nat refs) const {
		assert(false);
		return null;
	}

	CodeOutput *Arena::codeOutput(Binary *owner, LabelOutput *src) const {
		return codeOutput(owner, src->offsets, src->size, src->refs);
	}

	void Arena::removeFnRegs(RegSet *from) const {
		from->remove(ptrA);
		from->remove(ptrB);
		from->remove(ptrC);
	}

	Listing *Arena::redirect(Bool member, TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand param) {
		assert(false);
		return null;
	}

	Listing *Arena::engineRedirect(TypeDesc *result, Array<TypeDesc *> *params, Ref fn, Operand engine) {
		assert(false);
		return null;
	}

	Nat Arena::firstParamId(MAYBE(TypeDesc *) desc) {
		assert(false);
		if (desc)
			return 0;
		else
			return 1;
	}

	Operand Arena::firstParamLoc(Nat id) {
		assert(false);
		return Operand();
	}

	Reg Arena::functionDispatchReg() {
		assert(false);
		return noReg;
	}


#if defined(X86) && defined(WINDOWS)
	Arena *arena(EnginePtr e) {
		return new (e.v) x86::Arena();
	}
#elif defined(X64) && defined(POSIX)
	Arena *arena(EnginePtr e) {
		return new (e.v) x64::Arena();
	}
#elif defined(ARM64) && defined(POSIX)
	Arena *arena(EnginePtr e) {
		return new (e.v) arm64::Arena();
	}
#else
#error "Please note which is the default arena for your platform."
#endif
}
