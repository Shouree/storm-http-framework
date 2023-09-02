#pragma once

/**
 * Dummy "stdint" file, since VS 2008 does not have one.
 */

#ifdef _WIN32

typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#else

#error "Not supported outside of Windows builds!"

#endif
