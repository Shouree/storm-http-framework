#pragma once

/**
 * Bitwise math utilities.
 *
 * Assumes they are called with some *unsigned* type, preferrably 4 or 8 byte ones.
 */
#include <climits>
#include "Platform.h"

/**
 * Helper class for size-dependent specializations.
 */
template <class T, size_t numSize>
struct BitwiseImpl {};

template <class T>
struct BitwiseImpl<T, 1> {
	static nat trailingZeros(T v) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		static const byte vals[] = { 0xAA, 0xCC, 0xF0 };
		nat r = (v & vals[0]) != 0;
		for (nat i = 2; i > 0; i--)
			r |= ((v & vals[i]) != 0) << i;
		return r;
	}

	static nat setBitCount(T number) {
		nat v = number;
		v = ((v & 0xAA) >> 1) + (v & 0x55);
		v = ((v & 0xCC) >> 2) + (v & 0x33);
		v = ((v & 0xF0) >> 4) + (v & 0x0F);
		return v;
	}
};

template <class T>
struct BitwiseImpl<T, 4> {
	static nat trailingZeros(T v) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		static const nat vals[] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
		nat r = (v & vals[0]) != 0;
		for (nat i = 4; i > 0; i--)
			r |= ((v & vals[i]) != 0) << i;
		return r;
	}

	static nat setBitCount(T number) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		number = number - ((number >> 1) & 0x55555555);
		number = (number & 0x33333333) + ((number >> 2) & 0x33333333);
		return ((number + (number >> 4) & 0x0F0F0F0F) * 0x01010101) >> 24;
	}
};

template <class T>
struct BitwiseImpl<T, 8> {
	static nat trailingZeros(T v) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		static const nat64 vals[] = {
			0xAAAAAAAAAAAAAAAAULL, 0xCCCCCCCCCCCCCCCCULL, 0xF0F0F0F0F0F0F0F0ULL,
			0xFF00FF00FF00FF00ULL, 0xFFFF0000FFFF0000ULL, 0xFFFFFFFF00000000ULL
		};
		nat r = (v & vals[0]) != 0;
		for (nat i = 5; i > 0; i--)
			r |= ((v & vals[i]) != 0) << i;
		return r;
	}

	static nat setBitCount(T number) {
		// From Bit Twiddling Hacks: https://graphics.stanford.edu/~seander/bithacks.html
		number = number - ((number >> 1) & 0x5555555555555555ULL);
		number = (number & 0x3333333333333333ULL) + ((number >> 2) & 0x3333333333333333ULL);
		number = (number + (number >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
		return (number * 0x0101010101010101ULL) >> 56;
	}
};


// Check if a number is a power of two.
template <class T>
inline bool isPowerOfTwo(T number) {
	return (number & (number - 1)) == 0;
}

// Get the next larger power of two, or 'number' if it is already a power of two.
template <class T>
inline T nextPowerOfTwo(T number) {
	--number;

	for (size_t i = 1; i < sizeof(number) * CHAR_BIT; i *= 2)
		number |= number >> i;

	return number + 1;
}

// Get the number of trailing zeros.
template <class T>
inline nat trailingZeros(T number) {
	// Extract the least set bit, so that BitwiseImpl may assume it is a power of two.
	number &= T() - number;
	return BitwiseImpl<T, sizeof(T)>::trailingZeros(number);
}

// Count the number of set bits.
template <class T>
inline nat setBitCount(T number) {
	return BitwiseImpl<T, sizeof(T)>::setBitCount(number);
}

// Round up to the nearest multiple of "multiple"
// T should be an integer number, preferrably unsigned.
template <class T>
inline T roundUp(T number, T multiple) {
	T remainder = number % multiple;
	if (remainder == 0) return number;
	return number + (multiple - remainder);
}

// Round down to the nearest multiple of "multiple"
// T should be an integer number, preferrably unsigned.
template <class T>
inline T roundDown(T number, T multiple) {
	return number - (number % multiple);
}


/**
 * Compile-time computation of powers of two.
 */
template <size_t v, size_t iter = 1>
struct NextPowerOfTwo {
	static const size_t value = NextPowerOfTwo<((v - 1) | ((v - 1) >> iter)) + 1, iter + 1>::value;
};

template <size_t val>
struct NextPowerOfTwo<val, sizeof(size_t) * CHAR_BIT> {
	static const size_t value = val;
};


// Check multiply for overflow. Returns true if multiplication can be done without overflow.
#if defined(GCC)

inline bool multiplyOverflow(size_t a, size_t b, size_t &out) {
	return !__builtin_mul_overflow(a, b, &out);
}

#elif defined(VISUAL_STUDIO) && defined(X64)

#include <intrin.h>

inline bool multiplyOverflow(size_t a, size_t b, size_t &out) {
	unsigned __int64 high;
	out = _umul128(a, b, &high);
	return high == 0;
}

#else

#if defined(X86)

inline bool multiplyOverflow(size_t a, size_t b, size_t &out) {
	// Note: There are functions in the <intsafe.h> header for Win32. The compiler seems to
	// understand this well enough, so it is likely not worthwile to bother with.
	unsigned long long result = static_cast<unsigned long long>(a) * b;
	out = static_cast<size_t>(result);
	return out == result;
}

#elif defined(X64)

inline bool multiplyOverflow(size_t a, size_t b, size_t &out) {
	// On some platforms (ARM?), this might require a function call. Should be checked if/when we support ARM.
	unsigned __int128 result = static_cast<unsigned __int128>(a) * b;
	out = static_cast<size_t>(result);
	return out == result;
}

#else

#error "Unknown platform!"

#endif

#endif
