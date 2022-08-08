#include "stdafx.h"

#ifdef X86

int callFn(const void *fnPtr, int p) {
	Nat oldEsi = 0xDEADBEEF, oldEdi = 0xFEBA1298, oldEbx = 0x81732A98, oldEbp;
	Nat newEsi, newEdi, newEbx, newEbp;
	int rv;
	__asm {
		mov edi, oldEdi;
		mov esi, oldEsi;
		mov ebx, oldEbx;
		mov oldEbp, ebp;
		push p;
		call fnPtr;
		add esp, 4;
		mov rv, eax;
		mov newEbp, ebp;
		mov newEbx, ebx;
		mov newEsi, esi;
		mov newEdi, edi;
	}

	assert(oldEbp == newEbp);
	assert(oldEbx == newEbx);
	assert(oldEsi == newEsi);
	assert(oldEdi == newEdi);

	return rv;
}

int64 callFn(const void *fnPtr, int64 p) {
	Nat oldEsi = 0xDEADBEEF, oldEdi = 0xFEBA1298, oldEbx = 0x81732A98, oldEbp;
	Nat newEsi, newEdi, newEbx, newEbp;

	int ph = p >> 32;
	int pl = p & 0xFFFFFFFF;

	nat rvh;
	nat rvl;
	__asm {
		mov edi, oldEdi;
		mov esi, oldEsi;
		mov ebx, oldEbx;
		mov oldEbp, ebp;
		push ph;
		push pl;
		call fnPtr;
		add esp, 8;
		mov rvl, eax;
		mov rvh, edx;
		mov newEbp, ebp;
		mov newEbx, ebx;
		mov newEsi, esi;
		mov newEdi, edi;
	}

	assert(oldEbp == newEbp);
	assert(oldEbx == newEbx);
	assert(oldEsi == newEsi);
	assert(oldEdi == newEdi);

	nat64 r = (nat64(rvh) << 32) | nat64(rvl);
	return r;
}

#elif (defined(X64) || defined(ARM64)) && defined(GCC)
#include "CheckCall64.h"

int callFn(const void *fnPtr, int p) {
	Reg64 before = defaultReg64();
	Reg64 after = defaultReg64();

	int result = (int)checkCall(fnPtr, p, &after);

	assert(before == after, ::toS(before) + L" <=> " + ::toS(after));

	return result;
}

int64 callFn(const void *fnPtr, int64 p) {
	Reg64 before = defaultReg64();
	Reg64 after = defaultReg64();

	size_t result = checkCall(fnPtr, p, &after);

	assert(before == after, ::toS(before) + L" <=> " + ::toS(after));

	return result;
}

#endif
