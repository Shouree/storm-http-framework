#pragma once
#include "GcArray.h"

namespace storm {
	STORM_PKG(core);

	/**
	 * Converting to/from various encodings.
	 *
	 * All functions named 'convert' read from a null-terminated string and outputs the encoded data
	 * to 'to' until a maximum of 'maxCount' characters have been emitted (including the null
	 * character). The number of filled entries is returned, including the terminating null character.
	 * Note that encoding might be invalid if the convert functions run out of space.
	 */

	// Convert from 'char' to 'wchar'.
	size_t convert(const char *from, wchar *to, size_t maxCount);
	size_t convert(const char *from, size_t inputCount, wchar *to, size_t maxCount);
	GcArray<wchar> *toWChar(Engine &e, const char *from);
	GcArray<wchar> *toWChar(Engine &e, const char *from, size_t maxCount);

	// Convert from 'wchar' to 'char'.
	size_t convert(const wchar *from, char *to, size_t maxCount);
	GcArray<char> *toChar(Engine &e, const wchar *from);

#ifdef POSIX

	// Convert from 'wchar_t' to 'wchar'
	size_t convert(const wchar_t *from, wchar *to, size_t maxCount);
	GcArray<wchar> *toWChar(Engine &e, const wchar_t *from);

#endif
}
