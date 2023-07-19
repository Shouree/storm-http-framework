#pragma once

#ifdef WINDOWS

// Get TIB for the current thread.
extern "C" NT_TIB *getTIB();

#endif
