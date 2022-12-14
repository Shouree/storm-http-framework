#pragma once

#ifdef POSIX

namespace code {
	namespace eh {

		// Snippnet used to resume from exceptions. Not callable from C.
		extern "C" void posixResumeExceptionThunk();

	}
}

#endif
