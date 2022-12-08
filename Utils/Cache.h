#pragma once

#include "Platform.h"

#ifdef __cplusplus
extern "C" {
#else

#ifdef VISUAL_STUDIO
	// in c-mode, the inline keyword does not seem to be supported. Use 'static' instead, that is
	// good enough there.
#define inline static
#endif
#endif

/**
 * This file contains functions for managing the instruction- and data caches for communication
 * between multiple cores. In most cases, it is enough to utilize the atomic operations available in
 * the InlineAtomics header, but some times (especially when updating code), explicit cache
 * maintenance is necessary. These functions expand as necessary on each of our supported platforms.
 *
 * Note: This file is included from C, so we need to ensure it compiles from C.
 */

// Invalidates the data cache to ensure writes has reached memory and are visible to other cores.
inline void invalidateDCache(void *start, void *end);

// Make sure any pending data operations are done executing on the current thread.
inline void dataBarrier();

// Invalidate the instruction cache for an individual word. This function assumes that this thread
// has updated the instruction at "start", and that it needs to be flushed from the data cache
// first. After execution, the changes are immediately visible to other CPUs, but all CPUs need to
// call "clearLocalICache" first.
inline void invalidateSingleICache(void *start);

// Invalidate the instruction cache for a range of memory. The function assumes that this thread
// hass updated the code in the range and that it needs to be flushed from the data cache
// first. After execution, the changes are immediately visible to other CPUs, but all CPUs need to
// call "clearLocalICache" first.
inline void invalidateICache(void *start, void *end);

// Make sure that any changes to the ICache are respected on the current CPU. While the other ICache
// functions ensure that changes are visible, instructions may still be prefetched on this CPU. This
// function flushes the prefetch, and any other local caches that need to be updated.
inline void clearLocalICache();


#if defined(GCC)

#if defined(X86) || defined(X64)

// Nothing special needed on X86/X64 except for preventing reordering from the compiler.
inline void invalidateDCache(void *start, void *end) {
	(void)start;
	(void)end;
	__asm__ volatile ("" : : : "memory");
}

inline void dataBarrier() {
	__asm__ volatile ("" : : : "memory");
}

inline void invalidateSingleICache(void *start) {
	(void)start;
	__asm__ volatile ("" : : : "memory");
}

inline void invalidateICache(void *start, void *end) {
	(void)start;
	(void)end;
	__asm__ volatile ("" : : : "memory");
}

inline void clearLocalICache() {
	__asm__ volatile ("" : : : "memory");
}

#elif defined(ARM64)

// We need to do things here...
// The code here is from the "Arm Architecture Reference Manual", section K11.5 (Barrier Litmus Tests)

inline void invalidateDCache(void *start, void *end) {
	// We could probably do something cheaper here.
	__builtin___clear_cache(start, end);
}

inline void dataBarrier() {
	__asm__ volatile ("dsb ish\n\t" : : : "memory");
}

inline void clearLocalICache() {
	__asm__ volatile ("isb\n\t" : : : "memory");
}

inline void invalidateSingleICache(void *start) {
	__asm__ volatile (
		"dc cvau, %0\n\t"  // Clean data cache to point of unification.
		"dsb ish\n\t"      // Make sure previous operation is visible to all CPUs.
		"ic ivau, %0\n\t"  // Clean instruction cache to point of unification.
		"dsb ish\n\t"      // Make sure previous operation is visible to all CPUs.
		: : "r"(start)
		: "memory" );
}

inline void invalidateICache(void *start, void *end) {
	char *b = (char *)start;
	char *e = (char *)end;

	// Get cache sizes.
	unsigned int cache_info = 0;
	__asm__ volatile ("mrs %0, ctr_el0\n\t" : "=r" (cache_info));

	unsigned int icache = 4 << (cache_info & 0xF);
	unsigned int dcache = 4 << ((cache_info >> 16) & 0xF);

	// First, clear the data cache.
	for (char *at = b; at < e; at += dcache)
		__asm__ volatile ("dc cvau, %0\n\t" : : "r" (at));

	// Make sure it is visible. We wait with the memory barrier until here. We don't care in which
	// order the "dc cvau" instructions execute, the important thing is that they are all executed
	// before the "dsb ish" instruction here. That is why the memory barrier is here and nowhere else.
	__asm__ volatile ("dsb ish\n\t" : : : "memory");

	// Then, we clear the instruction cache.
	for (char *at = b; at < e; at += icache)
		__asm__ volatile ("ic ivau, %0\n\t" : : "r" (at));

	// Again, wait for the cleaning to be propagated properly.
	__asm__ volatile ("dsb ish\n\t" : : : "memory");
}

#endif


#elif defined(VISUAL_STUDIO)

// Note: On MSVC we currently only support X86, where we don't need explicit cache control.
#if !defined(X86) && !defined(X64)
#error "You likely need to implement cache invalidation for this architecture."
#endif

inline void invalidateDCache(void *start, void *end) {}
inline void invalidateSingleICache(void *start) {}
inline void invalidateICache(void *start, void *end) {}
inline void clearLocalICache() {}
inline void dataBarrier() {}


#endif

#ifdef __cplusplus
}
#else

#ifdef VISUAL_STUDIO
#undef inline
#endif

#endif
