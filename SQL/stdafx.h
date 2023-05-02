// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef STORM_SQL_H
#define STORM_SQL_H

#ifdef __cplusplus

#include "Shared/Storm.h"

namespace sql {

	using namespace storm;

}

#endif

#ifdef STORM_COMPAT
// Disable LFS so that we can run on systems without the fcntl64 function in glibc (Ubuntu 18.08 LTS).
// Will be removed soon.
#define SQLITE_DISABLE_LFS 1
#endif

#endif
