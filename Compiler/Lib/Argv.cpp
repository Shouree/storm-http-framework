#include "stdafx.h"
#include "Argv.h"
#include "Engine.h"

namespace storm {

	Array<Str *> *argv(EnginePtr e) {
		Array<Str *> *v = e.v.argv();
		if (v) {
			// Note: enough to copy the array itself, since strings are mutable.
			return new (e.v) Array<Str *>(*e.v.argv());
		} else {
			// No argv array was set. Create an empty one!
			return new (e.v) Array<Str *>();
		}
	}

	void argv(EnginePtr e, Array<Str *> *val) {
		// Note: enough to copy the array itself, since strings are mutable.
		e.v.argv(new (e.v) Array<Str *>(*val));
	}

}
