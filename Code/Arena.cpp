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

	void Arena::removeFnRegs(RegSet *from) const {
		from->remove(ptrA);
		from->remove(ptrB);
		from->remove(ptrC);
	}


#if defined(X86) && defined(WINDOWS)
	Arena *arena(EnginePtr e) {
		return new (e.v) x86::Arena();
	}
#elif defined(X64) && defined(WINDOWS)
	Arena *arena(EnginePtr e) {
		return new (e.v) x64::WindowsArena();
	}
#elif defined(X64) && defined(POSIX)
	Arena *arena(EnginePtr e) {
		return new (e.v) x64::PosixArena();
	}
#elif defined(ARM64) && defined(POSIX)
	Arena *arena(EnginePtr e) {
		return new (e.v) arm64::Arena();
	}
#else
#error "Please note which is the default arena for your platform."
#endif

	Binary *codeBinary(const void *fn) {
		// All backends do this.
		GcCode *refs = runtime::codeRefs((void *)fn);
		return (Binary *)refs->refs[0].pointer;
	}

}
