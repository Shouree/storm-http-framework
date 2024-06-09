#pragma once

#include "Indent.h"
#include <iostream>

using std::wostream;
using std::endl;

#define DEBUG_OUT

#ifdef DEBUG_OUT

// print line to debug output, usage:
// PLN("Hello " << "world!");
// PLN("Hello " << n << " times!");
// PLN_IF("X is larger than 10", X > 10);
#define PNN(str) util::debugStream() << str
#define PLN(str) util::debugStream() << str << std::endl
#define PLN_IF(str, cond) if (cond) PLN(str)
#define PVAR(expr) PLN(#expr << "=" << (expr))

// Good way of making TODO: comments (only displays 5 times).
// #define TODO(str) PLN("TODO("__FUNCTION__"):" << str)
#define TODO(str) do { static nat _times = 0; if (++_times <= 5) PLN("TODO(" << __FUNCTION__ << "): " << str); } while (false)

// Good way of indicating something possibly interesting during debugging.
#if defined(VISUAL_STUDIO)
#define WARNING(str) PLN("WARNING " << __FUNCTION__ << ": " << str);
#elif defined(GCC)
#define WARNING(str) PLN("WARNING " << __PRETTY_FUNCTION__ << ": " << str);
#endif

#include "Timer.h"
#define TIME(str) util::Timer __timer__(str);

#else
#define PNN(str)
#define PLN(str)
#define PLN_IF(str, cond)
#define PVAR(expr)
#define TIME(str)
#define WARNING(str)
#define TODO(str)

#endif

namespace util {
	// Get the debug stream.
	std::wostream &debugStream();
}


/**
 * Insanely useful for debugging who made bad deallocations:
 * 		_CrtSetAllocHook(&myHook);
 * static void *watch = 0;
 *
 * int myHook(int type, void *data, size_t size, int blockuse, long request, const byte *filename, int line) {
 * 	if (data == watch && type == _HOOK_FREE)
 * 		DebugBreak();
 * 	return TRUE;
 * }
 *	_ASSERT(_CrtCheckMemory());
 *
 */


/**
 * Initialize debug runtime.
 */
void initDebug();
