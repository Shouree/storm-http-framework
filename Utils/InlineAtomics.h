#pragma once

#include "Platform.h"

/**
 * Atomic operations for various platforms. Declared as inlined functions to make them cheaper.
 */


// Atomic increment/decrement.
size_t atomicIncrement(volatile size_t &v);
size_t atomicDecrement(volatile size_t &v);

// Bitwise operations.
size_t atomicAnd(volatile size_t &v, size_t with);
size_t atomicOr(volatile size_t &v, size_t with);
size_t atomicXor(volatile size_t &v, size_t with);

// Compare and swap.
size_t atomicCAS(volatile size_t &v, size_t compare, size_t exchange);
void *atomicCAS(void *volatile &v, void *compare, void *exchange);
template <class T>
T *atomicCAS(T *volatile &v, void *compare, void *exchange) {
	return (T *)atomicCAS((void *volatile&)v, compare, exchange);
}

// Atomic read/write. Mainly used to prevent compiler optimizations around accesses to a particular
// value wrt other reads/writes.
size_t atomicRead(volatile const size_t &v);
void *atomicRead(void *volatile const &v);
void atomicWrite(volatile size_t &v, size_t value);
void atomicWrite(void *volatile &v, void *value);
template <class T>
void atomicWrite(T *volatile &v, T *value) {
	atomicWrite((void *volatile&)v, value);
}
template <class T>
T *atomicRead(T *volatile const &v) {
	return (T *)atomicRead((void *volatile const&)v);
}

// Atomic read/write for unaligned data (at least we're making our best attempt).
size_t unalignedAtomicRead(volatile const size_t &v);
void unalignedAtomicWrite(volatile size_t &v, size_t value);
void shortUnalignedAtomicWrite(volatile nat &v, nat value);


// Special case implementations for 64-bit machines where sizeof(size_t) != sizeof(nat)
#if defined(X64) || defined(ARM64)
nat atomicIncrement(volatile nat &v);
nat atomicDecrement(volatile nat &v);
nat atomicCAS(volatile nat &v, nat compare, nat exchange);
nat atomicRead(volatile const nat &v);
void atomicWrite(volatile nat &v, nat value);
#endif



/**
 * Implementation for Win32.
 */
#if defined(WINDOWS)
#include <intrin.h>

#ifdef X64
#error "Revise atomics for 64-bit!"

inline nat atomicIncrement(volatile nat &v) {
	return (size_t)_InterlockedIncrement((volatile LONG *)&v);
}

inline nat atomicDecrement(volatile nat &v) {
	return (size_t)_InterlockedDecrement((volatile LONG *)&v);
}

inline size_t atomicIncrement(volatile size_t &v) {
	return (size_t)_InterlockedIncrement64((volatile LONG64 *)&v);
}

inline size_t atomicDecrement(volatile size_t &v) {
	return (size_t)_InterlockedDecrement64((volatile LONG64 *)&v);
}

inline size_t atomicAnd(volatile size_t &v, size_t with) {
	return (size_t)_InterlockedAnd64((volatile LONG64 *)&v, with);
}

inline size_t atomicOr(volatile size_t &v, size_t with) {
	return (size_t)_InterlockedOr64((volatile LONG64 *)&v, with);
}

inline size_t atomicXor(volatile size_t &v, size_t with) {
	return (size_t)_InterlockedXor64((volatile LONG64 *)&v, with);
}

inline nat atomicRead(volatile const nat &v) {
	_ReadWriteBarrier();
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	nat r = v;
	_ReadWriteBarrier();
	return r;
}

inline void atomicWrite(volatile nat &v, nat value) {
	_ReadWriteBarrier();
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
	_ReadWriteBarrier();
}

// Note: The InterlockedCompareExchangePointer does is not inlined in VS 2008.
inline size_t atomicCAS(volatile size_t &v, size_t compare, size_t exchange) {
	return (size_t)_InterlockedCompareExchange64(&v, exchange, compare);
}

inline void *atomicCAS(void *volatile &v, void *compare, void *exchange) {
	return _InterlockedCompareExchange64(&v, exchange, compare);
}

#else

inline size_t atomicIncrement(volatile size_t &v) {
	return (size_t)_InterlockedIncrement((volatile LONG *)&v);
}

inline size_t atomicDecrement(volatile size_t &v) {
	return (size_t)_InterlockedDecrement((volatile LONG *)&v);
}

inline size_t atomicAnd(volatile size_t &v, size_t with) {
	return (size_t)_InterlockedAnd((volatile LONG *)&v, with);
}

inline size_t atomicOr(volatile size_t &v, size_t with) {
	return (size_t)_InterlockedOr((volatile LONG *)&v, with);
}

inline size_t atomicXor(volatile size_t &v, size_t with) {
	return (size_t)_InterlockedXor((volatile LONG *)&v, with);
}

// Note: The InterlockedCompareExchangePointer does is not inlined in VS 2008.
inline size_t atomicCAS(volatile size_t &v, size_t compare, size_t exchange) {
	return _InterlockedCompareExchange((long volatile *)&v, exchange, compare);
}

inline void *atomicCAS(void *volatile &v, void *compare, void *exchange) {
	return (void *)_InterlockedCompareExchange((long volatile *)&v, (size_t)exchange, (size_t)compare);
}

#endif

inline size_t atomicRead(volatile const size_t &v) {
	_ReadWriteBarrier();
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	return v;
}

inline void *atomicRead(void *volatile const &v) {
	_ReadWriteBarrier();
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	void *r = v;
	_ReadWriteBarrier();
	return r;
}

inline void atomicWrite(volatile size_t &v, size_t value) {
	_ReadWriteBarrier();
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
	_ReadWriteBarrier();
}

inline void atomicWrite(void *volatile &v, void *value) {
	_ReadWriteBarrier();
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
	_ReadWriteBarrier();
}

#ifdef X86

inline size_t unalignedAtomicRead(volatile const size_t &v) {
	volatile const size_t *addr = &v;
	size_t result;
	__asm {
		mov eax, 0;
		mov ecx, addr;
		lock add eax, [ecx];
		mov result, eax;
	}
	return result;
}

inline void unalignedAtomicWrite(volatile size_t &v, size_t value) {
	volatile size_t *addr = &v;
	__asm {
		mov eax, value;
		mov ecx, addr;
		lock xchg [ecx], eax;
	}
}

inline void shortUnalignedAtomicWrite(volatile nat &v, nat value) {
	volatile nat *addr = &v;
	__asm {
		mov eax, value;
		mov ecx, addr;
		lock xchg [ecx], eax;
	}
}

#else
#error "Implement unaligned atomics for X86-64 as well!"
#endif

#elif defined(GCC)

#define BARRIER asm volatile ("" ::: "memory")

inline size_t atomicIncrement(volatile size_t &v) {
	return __atomic_add_fetch(&v, 1, __ATOMIC_ACQ_REL);
}

inline size_t atomicDecrement(volatile size_t &v) {
	return __atomic_sub_fetch(&v, 1, __ATOMIC_ACQ_REL);
}

inline size_t atomicAnd(volatile size_t &v, size_t with) {
	return __atomic_and_fetch(&v, with, __ATOMIC_ACQ_REL);
}

inline size_t atomicOr(volatile size_t &v, size_t with) {
	return __atomic_or_fetch(&v, with, __ATOMIC_ACQ_REL);
}

inline size_t atomicXor(volatile size_t &v, size_t with) {
	return __atomic_xor_fetch(&v, with, __ATOMIC_ACQ_REL);
}

inline size_t atomicCAS(volatile size_t &v, size_t compare, size_t exchange) {
	__atomic_compare_exchange_n(&v, &compare, exchange, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
	// Result is stored in "compare".
	return compare;
}

inline void *atomicCAS(void *volatile &v, void *compare, void *exchange) {
	__atomic_compare_exchange_n(&v, &compare, exchange, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
	// Result is stored in "compare".
	return compare;
}

#if defined(X64) || defined(ARM64) // 64-bit architectures

inline nat atomicIncrement(volatile nat &v) {
	return __atomic_add_fetch(&v, 1, __ATOMIC_ACQ_REL);
}

inline nat atomicDecrement(volatile nat &v) {
	return __atomic_sub_fetch(&v, 1, __ATOMIC_ACQ_REL);
}

inline nat atomicCAS(volatile nat &v, nat compare, nat exchange) {
	__atomic_compare_exchange_n(&v, &compare, exchange, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
	// Result is stored in "compare".
	return compare;
}

#endif

#ifdef X64

inline nat atomicRead(volatile const nat &v) {
	BARRIER;
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	nat r = v;
	BARRIER;
	return r;
}

inline void atomicWrite(volatile nat &v, nat value) {
	BARRIER;
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
	BARRIER;
}

#endif

#if defined(X86) || defined(X64)

inline size_t atomicRead(volatile const size_t &v) {
	BARRIER;
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	size_t r = v;
	BARRIER;
	return r;
}

inline void *atomicRead(void *volatile const &v) {
	BARRIER;
	// Volatile reads are atomic on X86/X64 as long as they are aligned.
	void *r = v;
	BARRIER;
	return r;
}

inline void atomicWrite(volatile size_t &v, size_t value) {
	BARRIER;
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
	BARRIER;
}

inline void atomicWrite(void *volatile &v, void *value) {
	BARRIER;
	// Volatile writes are atomic on X86/X64 as long as they are aligned.
	v = value;
	BARRIER;
}

#endif

#if defined(ARM64)

inline nat atomicRead(volatile const nat &v) {
	return __atomic_load_n(&v, __ATOMIC_ACQUIRE);
}

inline void atomicWrite(volatile nat &v, nat value) {
	__atomic_store_n(&v, value, __ATOMIC_RELEASE);
}

inline size_t atomicRead(volatile const size_t &v) {
	return __atomic_load_n(&v, __ATOMIC_ACQUIRE);
}

inline void *atomicRead(void *volatile const &v) {
	return __atomic_load_n(&v, __ATOMIC_ACQUIRE);
}

inline void atomicWrite(volatile size_t &v, size_t value) {
	__atomic_store_n(&v, value, __ATOMIC_RELEASE);
}

inline void atomicWrite(void *volatile &v, void *value) {
	__atomic_store_n(&v, value, __ATOMIC_RELEASE);
}

#endif

#if defined(X86)

inline size_t unalignedAtomicRead(volatile size_t &v) {
	size_t result;
	asm (
		"movl $0, %%eax\n\t"
		"movl %[addr], %%ecx\n\t"
		"lock addl (%%ecx), %%eax\n\t"
		"movl %%eax, %[result]\n\t"
		: [result] "=r"(result)
		: [addr] "r"(&v)
		: "eax", "ecx", "memory");
	return result;
}

inline void unalignedAtomicWrite(volatile size_t &v, size_t value) {
	asm (
		"movl %[value], %%eax\n\t"
		"movl %[addr], %%ecx\n\t"
		"lock xchgl %%eax, (%%ecx)\n\t"
		:
		: [addr] "r"(&v), [value] "r"(value)
		: "eax", "ecx", "memory");
}

inline void shortUnalignedAtomicWrite(volatile nat &v, nat value) {
	asm (
		"movl %[value], %%eax\n\t"
		"movl %[addr], %%ecx\n\t"
		"lock xchgl %%eax, (%%ecx)\n\t"
		:
		: [addr] "r"(&v), [value] "r"(value)
		: "eax", "ecx", "memory");
}

#elif defined(X64)

inline size_t unalignedAtomicRead(volatile size_t &v) {
	size_t result;
	asm (
		"movq %[addr], %%rcx\n\t"
		// "movq $0, %%rax\n\t"
		// "lock addq (%%rcx), %%rax\n\t"
		"movq (%%rcx), %%rax\n\t" // TODO: It seems X86 will make sure this is atomic...
		"movq %%rax, %[result]\n\t"
		: [result] "=r"(result)
		: [addr] "r"(&v)
		: "rax", "rcx", "memory");
	return result;
}

inline void unalignedAtomicWrite(volatile size_t &v, size_t value) {
	asm (
		"movq %[value], %%rax\n\t"
		"movq %[addr], %%rcx\n\t"
		"lock xchgq %%rax, (%%rcx)\n\t"
		:
		: [addr] "r"(&v), [value] "r"(value)
		: "rax", "rcx", "memory");
}

inline void shortUnalignedAtomicWrite(volatile nat &v, nat value) {
	asm (
		"movl %[value], %%eax\n\t"
		"movq %[addr], %%rcx\n\t"
		"lock xchgl %%eax, (%%rcx)\n\t"
		:
		: [addr] "r"(&v), [value] "r"(value)
		: "rax", "rcx", "memory");
}

#elif defined(ARM64)

// Note: Only included where needed, as it makes compilation on Visual Studio fail for some reason.
#include "Assert.h"

inline size_t unalignedAtomicRead(volatile size_t &v) {
	// Not supported at all...
	assert(false, "Unaligned access not supported on ARM.");
}

inline void unalignedAtomicWrite(volatile size_t &v, size_t value) {
	// Not supported at all...
	assert(false, "Unaligned access not supported on ARM.");
}

inline void unalignedAtomicWrite(volatile nat &v, nat value) {
	// Not supported at all...
	assert(false, "Unaligned access not supported on ARM.");
}

inline void shortUnalignedAtomicWrite(volatile nat &v, nat value) {
	assert(false, "Unaligned access not supported on ARM.");
}

#else
#error "Unaligned operations not supported for this architecture yet."
#endif

#else
#error "Atomic operations are only supported for win32 and GCC for now."
#endif

