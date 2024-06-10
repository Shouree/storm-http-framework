#pragma once
#include "Value.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Result from an expression. A result either returns a Value (which may be void), or a special
	 * value indicating that the expression will never return for some reason. For example, if a
	 * return statement has been found in a block (on all paths), that block will return this *no
	 * return* value. The same holds true for paths always throwing an exception.
	 *
	 * Note the difference between *never returns* and *returns void*. Because it is easy to confuse
	 * these concepts, this class does not provide the usual `any` and `empty` members. Instead,
	 * `nothing`, `empty` and `value` are provided.
	 */
	class ExprResult {
		STORM_VALUE;
	public:
		// Return void.
		STORM_CTOR ExprResult();

		// Return a value.
		STORM_CAST_CTOR ExprResult(Value result);

		// Result type. If this is the *no-return* value, then `void` is returned here.
		Value STORM_FN type() const;

		// Does the expression return a value (ie. not `void`)?
		Bool STORM_FN value() const;

		// Does the expression return void?
		Bool STORM_FN empty() const;

		// Does the expression not return at all.
		Bool STORM_FN nothing() const;

		// Same type?
		Bool STORM_FN operator ==(const ExprResult &o) const;
		Bool STORM_FN operator !=(const ExprResult &o) const;

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Convert the result to a by-reference value.
		ExprResult STORM_FN asRef(Bool v) const;

		// Output.
		void STORM_FN toS(StrBuf *to) const;

	private:
		// Return type.
		Value result;

		// Any result?
		Bool returns;

		// Create 'no-return' value.
		ExprResult(Bool any);

		// Allow 'noReturn' to create instances.
		friend ExprResult noReturn();
	};

	// Output.
	wostream &operator <<(wostream &to, const ExprResult &r);

	// Get the 'no return' value.
	ExprResult STORM_FN noReturn();

	/**
	 * Compute the common denominator of two values so that it is possible to cast both 'a' and 'b'
	 * to the resulting type. In case 'a' and 'b' are unrelated, Value() - void is returned. This
	 * also handles cases where either 'a' or 'b' never returns. In the case one of them never
	 * returns, the other value is returned unmodified. If both never returns, 'noReturn' is returned.
	 */
	ExprResult STORM_FN common(ExprResult a, ExprResult b);


}
