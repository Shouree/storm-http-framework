#pragma once
#include "Utils/Platform.h"

namespace code {
	namespace dwarf {

		/**
		 * Generic register numbers.
		 * Note: on X86-64 these correspond to EAX and EDX
		 * Note: on Arm64 these correspond to r0 and r1
		 */
#define DWARF_EH_RETURN_0 __builtin_eh_return_data_regno(0)
#define DWARF_EH_RETURN_1 __builtin_eh_return_data_regno(1)

		/**
		 * Frame pointer for the current platform.
		 */
#if defined(X64) || defined(X86)
#define DWARF_EH_FRAME_POINTER 6
#elif defined(ARM64)
#define DWARF_EH_FRAME_POINTER 29
#endif

	}
}
