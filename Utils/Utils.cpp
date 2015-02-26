// stdafx.cpp : source file that includes just the standard includes
// Spel.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "Platform.h"

#ifdef WINDOWS

// Check alignment of value.
static inline bool aligned(volatile void *v) {
	UINT_PTR i = (UINT_PTR)v;
	return (i & 0x3) == 0;
}

nat atomicIncrement(volatile nat &v) {
	assert(aligned(&v));
	return (nat)InterlockedIncrement((volatile LONG *)&v);
}

nat atomicDecrement(volatile nat &v) {
	assert(aligned(&v));
	return (nat)InterlockedDecrement((volatile LONG *)&v);
}

nat atomicCAS(volatile nat &v, nat compare, nat exchange) {
	assert(aligned(&v));
	return (nat)InterlockedCompareExchange((volatile LONG *)&v, (LONG)exchange, (LONG)compare);
}


#else
#error "atomicIncrement and atomicDecrement are only supported on Windows for now"
#endif
