#pragma once

namespace code {
	namespace x64 {

		// X64 resume exception snippnet. Not callable from other contexts.
		extern "C" void x64ResumeExceptionThunk();

	}
}
