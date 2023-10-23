#pragma once
#include "Text.h"
#include "Core/Str.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Read characters from a string.
	 */
	class StrInput : public TextInput {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR StrInput(Str *src);

	protected:
		virtual Char STORM_FN readChar();

	private:
		Str::Iter pos;
		Str::Iter end;
	};

}
