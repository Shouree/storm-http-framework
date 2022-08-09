#pragma once

#include "Platform.h"

#if defined(VISUAL_STUDIO) && defined(X86)

/**
 * On X86 on Windows, pointers to member functions are always pointers. If they refer to a virtual
 * function, they simply point to a piece of code that performs the virtual dispatch.
 */

template <class T>
inline const void *address(T fn) {
	return (const void *&)fn;
}

template <class Fn>
inline Fn asMemberPtr(const void *fn) {
	Fn result;
	memset(&result, 0, sizeof(result));
	memcpy(&result, &fn, sizeof(fn));
	return result;
}

#elif defined(VISUAL_STUDIO) && defined(X64)

/**
 * This is currently unexplored, but likely works as above.
 */

#error "Implement this!"

#elif defined(GCC) && defined(POSIX) && defined(X64)

/**
 * On GCC for Unix and X64, a member function pointer is actually 2 machine words. The first
 * (64-bit) word is where the actual pointer is located, while the second word contains an
 * offset to the vtable. Since we truncate function pointers to 1 machine word, the vtable
 * pointer is unfortunatly lost. This is, however, fine in Storm since we know that the
 * vtable is always located at offset 0. This is known since all objects with virtual
 * function inherit from an object for which this is true, and we do not support multiple
 * (virtual) inheritance.
 *
 * GCC abuses the function pointer a bit with regards to virtual dispatch as well. The
 * function pointer is either a plain pointer to the machine code to execute (always aligned
 * to at least 2 bytes) or an offset into the vtable of the object. The first case is easy:
 * just call the function at the other end of the pointer. In case the function pointer is a
 * vtable offset, we need to perform the vtable lookup through the object's vtable. We can
 * distinguish between the two cases by examining the least significant bit of the
 * pointer. If it is set (ie. if the address is odd), the pointer is actually an offset into
 * the vtable +1. Otherwise it is a pointer.
 *
 * This was derived by examining the assembler output from GCC when compiling the file
 * Experiments/membercall.cpp with 'g++ -S membercall.cpp'.
 */

template <class T>
inline const void *address(T fn) {
	return reinterpret_cast<const void *>(fn);
}

template <class Fn>
inline Fn asMemberPtr(const void *fn) {
	Fn result;
	memset(&to, 0, sizeof(Fn));
	memcpy(&to, &fn, sizeof(void *));
}

#elif defined(GCC) && defined(POSIX) && defined(ARM64)

/**
 * On GCC for Unix and ARM64, a member function pointer is 2 machine words as on X64. On
 * ARM, however, the first word is always the pointer (due to pointer authentication, I
 * think), and the offset is tagged instead.
 *
 * As on X64, we can assume that the offset is zero.
 *
 * Since we need to keep track of whether or not pointers refer to vtable offsets, we use
 * the same representation as on X64 for our void pointers (we don't use pointer
 * authentication, nor THUMB instructions, so we can assume pointers are aligned).
 */

template <class T>
inline const void *address(T fn) {
	struct {
		size_t data;
		size_t offset;
	} ptr;
	if (sizeof(fn) >= sizeof(ptr)) {
		memcpy(&ptr, &fn, sizeof(ptr));
		// Move the tag from "offset" into "data".
		ptr.data |= ptr.offset & 0x1;
	} else {
		memcpy(&ptr, &fn, sizeof(fn));
		// No tag in this case, just a regular pointer.
	}

	return reinterpret_cast<const void *>(ptr.data);
}

template <class Fn>
inline Fn asMemberPtr(const void *fn) {
	struct {
		size_t data;
		size_t offset;
	} ptr;

	// Extract the tag back into the offset.
	ptr.data = size_t(fn) & ~size_t(1);
	ptr.offset = size_t(fn) & 0x1;

	Fn result;
	memset(&result, 0, sizeof(result));
	memcpy(&result, &ptr, min(sizeof(result), sizeof(ptr)));
	return result;
}


#else

#error "Please implement handling of member pointers for your platform!"

#endif

