#pragma once
#include "Utils/Platform.h"

#if defined(WINDOWS) && defined(X86)

// Note: this declaration is a lie. Do not call directly.
extern "C"
void __stdcall x86SafeSEH();

// Shim called from the exception handler. Reads parameters to the 'x86SEHCleanup' in Seh.cpp from
// registers. Once again, the declaration is a lie. Do not call directly.
extern "C"
void __stdcall x86HandleException();

// Shim to call RtlUnwind as needed.
extern "C"
void __cdecl x86Unwind(_EXCEPTION_RECORD *er, void *targetFrame);

#else

// Fallback to make the code compile on other platforms.
inline void x86SafeSEH() {}

#endif
