#pragma once

#if defined(WINDOWS) && defined(X64)

namespace code {
	namespace eh {

		RUNTIME_FUNCTION *exceptionCallback(void *pc, void *base);

	}
}

#endif
