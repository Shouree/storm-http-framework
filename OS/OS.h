#pragma once
#include "Utils/Utils.h"
#include "Utils/Platform.h"

// Make sure the current CPU architecture is supported.
#if !defined(X86) && !defined(X64) && !defined(ARM64)
#error "Unsupported architecture, currently only x86, x86-64 and ARM64 are supported."
#endif

#include "Utils/Printable.h"
#include "Utils/Memory.h"
