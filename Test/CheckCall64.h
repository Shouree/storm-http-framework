#pragma once
#include "Utils/Platform.h"

#if defined(X64)

struct Reg64 {
	size_t rbx; // @0
	size_t r12; // @8
	size_t r13; // @16
	size_t r14; // @24
	size_t r15; // @32
};

inline Reg64 defaultReg64() {
	Reg64 r = {
		0x1111111111111111,
		0x2222222222222222,
		0x3333333333333333,
		0x4444444444444444,
		0x5555555555555555,
	};
	return r;
}

inline bool operator ==(const Reg64 &a, const Reg64 &b) {
	return a.rbx == b.rbx
		&& a.r12 == b.r12
		&& a.r13 == b.r13
		&& a.r14 == b.r14
		&& a.r15 == b.r15;
}

inline bool operator !=(const Reg64 &a, const Reg64 &b) {
	return !(a == b);
}

inline wostream &operator <<(wostream &to, const Reg64 &r) {
	return to << L"rbx: " << (void *)r.rbx
			  << L", r12: " << (void *)r.r12
			  << L", r13: " << (void *)r.r13
			  << L", r14: " << (void *)r.r14
			  << L", r15: " << (void *)r.r15;
}

extern "C" size_t checkCall(const void *fn, size_t param, Reg64 *regs);

#elif defined(ARM64)

struct Reg64 {
	size_t x;
};

inline Reg64 defaultReg64() {
	Reg64 r = {
		0x1111111111111111,
	};
	return r;
}

inline bool operator ==(const Reg64 &a, const Reg64 &b) {
	// return a.x == b.x;
	return false;
}

inline bool operator !=(const Reg64 &a, const Reg64 &b) {
	return !(a == b);
}

inline wostream &operator <<(wostream &to, const Reg64 &r) {
	return to << L"Check call not implemented yet!";
}

extern "C" size_t checkCall(const void *fn, size_t param, Reg64 *regs);

#endif
