#pragma once

#include "Utils/Platform.h"

#if defined(STORM_COMPAT) && defined(LINUX)

/**
 * To be compatible with older glibc (version 2.27 or earlier), that did not have a separate fcntl64
 * function, we define our own wrapper that is capable enough to do what sqlite needs to have.
 *
 * On older glibc, there was only one fcntl() call. That call differentiated between 32-bit and
 * 64-bit struct flock by having the F_GETLK, F_SETLK and F_SETLKW defined as F_SETLK64, etc. This
 * was changed in 2.28, so that fcntl() now refer to fcntl64() instead, which handles these
 * differences. Thus, we write this wrapper ourselves to stay compatible.
 *
 * It seems like off_t is always 64-bit on 64-bit machines, so F_SETLK == F_SETLK64 etc.
 */


// Definitions from SQLite, so that we can include system headers correctly.

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define _LARGE_FILE 1
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#define _LARGEFILE_SOURCE 1

// We need to include fcntl.h now, otherwise we can't re-define fcntl with macros later.
#include <fcntl.h>

extern int sqlite_fcntl_wrap(int fd, int cmd, ...);

// Now remap fcntl to our wrapper. It is implemented in "compat.cpp".
// Note: sometimes glibc uses a macro to alias fcntl to fcntl64, sometimes a symbol alias.
#ifdef fcntl
#undef fcntl
#endif

#define fcntl sqlite_fcntl_wrap

#endif
