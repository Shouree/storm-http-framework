#pragma once
#include "Core/Array.h"
#include "Core/Str.h"

namespace storm {
	STORM_PKG(core);

	// Access to command-line arguments of the running Storm engine.
	// This function returns a copy of the argument array.
	Array<Str *> *STORM_FN argv(EnginePtr e);

	// Set the arguments to the command line. This is allowed from any thread in the system. While
	// this will not result in a crash, it might not result in the desired behaviors.
	void STORM_ASSIGN argv(EnginePtr e, Array<Str *> *val);

}
