#pragma once
#include "Code/Reg.h"

#if defined(WINDOWS) && defined(X64)

namespace code {
	namespace eh {

		RUNTIME_FUNCTION *exceptionCallback(void *pc, void *base);

	}
}

#endif

/**
 * Platform-specific defines useful in other places:
 *
 * Based on documentation from here:
 * https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64
 */

namespace code {
	namespace eh {

		/**
		 * Same as RUNTIME_FUNCTION in the Win32 headers.
		 *
		 * Contents are pointers relative to the start of the image. In our case, the start of the
		 * image is the start of the allocation made by the active garbage collector.
		 */
		struct RuntimeFunction {
			Nat functionStart;
			Nat functionEnd;
			Nat unwindInfo;
		};


		/**
		 * Same as UNWIND_INFO in Win32 documentation (not present in Win32 headers).
		 *
		 * Note: The structure ends with a dynamic part (unwindCodes). This is not a part of the
		 * structure since it continues with more data afterwards.
		 */
		struct UnwindInfo {
			byte version : 3;
			byte flags : 5;
			byte prologSize;
			byte unwindCount;
			byte frameRegister : 4;
			byte frameOffset : 4;
			// unsigned short unwindCodes[x];
			// Nat handlerAddress
		};


		/**
		 * Constants for the "flags" in unwind info. Not present in Win32 headers, but we use a
		 * different name in case they will become available.
		 *
		 * UnwindFlagExamine specifies that the handler should be called in the first pass, when
		 * looking for a handler.
		 *
		 * UnwindFlagUnwind specifies that the handler should be called in the second pass, when
		 * unwinding the stack.
		 */
		static const byte UnwindFlagExamine = 0x1; // UNW_FLAG_EHANDLER
		static const byte UnwindFlagUnwind = 0x2; // UNW_FLAG_UHANDLER


		/**
		 * Unwind operations. Not present in Win32 headers, but uses a different naming scheme in
		 * case they appear.
		 */
		static const byte UnwindPushNonvol = 0x0;
		static const byte UnwindAllocSmall = 0x2;
		static const byte UnwindAllocLarge = 0x1;


		/**
		 * Register numbering on 64-bit Windows.
		 */
		Nat win64Register(Reg reg);


	}
}
