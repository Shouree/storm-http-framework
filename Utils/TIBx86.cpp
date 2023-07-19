#include "stdafx.h"
#include "TIB.h"

#ifdef WINDOWS
#ifdef X86

NT_TIB *getTIB() {
	NT_TIB *tib;
	__asm {
		// read "self" from the TIB.
		mov eax, fs:0x18;
		mov tib, eax;
	}
	// assert(tib == tib->Self);
	return tib;
}

#endif
#endif
