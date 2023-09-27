#include "stdafx.h"
#include "Io.h"
#include "Engine.h"

namespace storm {
	namespace io {

		void print(Str *s) {
			stdOut(s->engine())->writeLine(s);
		}

		TextInput *stdIn(EnginePtr e) {
			return e.v.stdIn();
		}

		TextOutput *stdOut(EnginePtr e) {
			return e.v.stdOut();
		}

		TextOutput *stdError(EnginePtr e) {
			return e.v.stdError();
		}

		void stdIn(EnginePtr e, TextInput *s) {
			e.v.stdIn(s);
		}

		void stdOut(EnginePtr e, TextOutput *s) {
			e.v.stdOut(s);
		}

		void stdError(EnginePtr e, TextOutput *s) {
			e.v.stdError(s);
		}

	}
}
