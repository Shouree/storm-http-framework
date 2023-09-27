#pragma once
#include "Core/Str.h"
#include "Core/EnginePtr.h"
#include "Core/Io/Text.h"

namespace storm {
	namespace io {
		STORM_PKG(core);

		/**
		 * Standard IO for use from Storm.
		 */

		// Quick print line function.
		void STORM_FN print(Str *s) ON(Compiler);

		// Get standard IO streams.
		TextInput *STORM_FN stdIn(EnginePtr e) ON(Compiler);
		TextOutput *STORM_FN stdOut(EnginePtr e) ON(Compiler);
		TextOutput *STORM_FN stdError(EnginePtr e) ON(Compiler);

		// Set standard IO streams.
		void STORM_FN stdIn(EnginePtr e, TextInput *s) ON(Compiler);
		void STORM_FN stdOut(EnginePtr e, TextOutput *s) ON(Compiler);
		void STORM_FN stdError(EnginePtr e, TextOutput *s) ON(Compiler);

	}
}
