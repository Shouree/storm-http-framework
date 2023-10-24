#pragma once

#include "Core/Io/Url.h"
#include "Core/Array.h"

namespace storm {
	STORM_PKG(core.sys);

	// Open an `Url` using the default program that is associated with the file in the operating
	// system. Throws a `SystemError` on failure.
	void STORM_FN open(Url *file);

	// Execute the program `program` with the parameters in `params`. If `program` is a relative url
	// that consists of a single part, then the function searches for the program in the `PATH`
	// variable.
	void STORM_FN execute(Url *program, Array<Str *> *params);

	// Execute the program `program` with the parameters in `params`. Start the program in the
	// specified working directory if specified. If `program` is a relative url that consists of a
	// single part, then the function searches for the program in the `PATH` variable.
	void STORM_FN execute(Url *program, Array<Str *> *params, MAYBE(Url *) workingDirectory);

}
