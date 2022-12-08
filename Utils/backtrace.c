#include "stdafx.h"
#include "backtrace.h"

#if defined(POSIX) && !defined(NO_LIBBACKTRACE)

// Seems to be needed to build properly. Likely would be added by the configure script.
#include <link.h>

// Include all files required by the backtrace library, so we do not have to compile it separatly.
#include "Linux/backtrace/atomic.c"
#include "Linux/backtrace/backtrace.c"
#include "Linux/backtrace/dwarf.c"
#include "Linux/backtrace/elf.c"
#include "Linux/backtrace/fileline.c"
#include "Linux/backtrace/mmap.c"
#include "Linux/backtrace/mmapio.c"
#include "Linux/backtrace/posix.c"
#include "Linux/backtrace/print.c"
#include "Linux/backtrace/simple.c"
#include "Linux/backtrace/sort.c"
#include "Linux/backtrace/state.c"

#endif
