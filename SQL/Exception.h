#pragma once
#include "Core/Exception.h"
#include "Core/Maybe.h"

namespace sql {

	class EXCEPTION_EXPORT SQLError : public storm::Exception {
		STORM_EXCEPTION;
	public:
		STORM_CTOR SQLError(Str *msg);
		STORM_CTOR SQLError(Str *msg, Maybe<Nat> code);

		virtual void STORM_FN message(StrBuf *to) const;

		// Error code.
		Maybe<Nat> code;

	private:
		Str *msg;
	};

}
