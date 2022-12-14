#pragma once

#ifdef POSIX
#include <unwind.h>
#else
// Make sure we can compile on other platforms. They need a pointer to 'stormPersonality' to link
// properly.
typedef int _Unwind_Reason_Code;
typedef int _Unwind_Action;
typedef int _Unwind_Exception_Class;
typedef int _Unwind_Exception;
typedef int _Unwind_Context;
#endif

namespace code {
	namespace eh {

		// Personality function used for Storm.
		_Unwind_Reason_Code stormPersonality(int version,
											_Unwind_Action action,
											_Unwind_Exception_Class type,
											_Unwind_Exception *data,
											_Unwind_Context *context);


	}
}

