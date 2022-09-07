#pragma once

#include "Platform.h"

#if defined(GCC)

static inline void invalidateCache(void *start, void *end) {
	__builtin___clear_cache(start, end);
}

#else

static inline void invalidateCache(void *start, size_t count) {
	// Not currently needed.
}

#endif

static inline void invalidateCache(void *start, size_t count) {
	invalidateCache(start, (byte *)start + count);
}
