#include "stdafx.h"
#include "Condition.h"
#include "WeakCast.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {

		/**
		 * Condition.
		 */

		Condition::Condition() {}

		SrcPos Condition::pos() {
			return SrcPos();
		}

		MAYBE(LocalVar *) Condition::result() {
			return null;
		}

		void Condition::trueCode(CodeGen *) {}


		/**
		 * Boolean condition.
		 */

		BoolCondition::BoolCondition(Expr *expr) : expr(expr) {
			if (expr->result().type().asRef(false) != Value(StormInfo<Bool>::type(engine())))
				throw new (this) TypeError(expr->pos, S("The expression must evaluate to Bool."));
		}

		SrcPos BoolCondition::pos() {
			return expr->largePos();
		}

		MAYBE(LocalVar *) BoolCondition::result() {
			// We never create anything!
			return null;
		}

		void BoolCondition::code(CodeGen *state, CodeResult *ok) {
			expr->code(state, ok);
		}

		void BoolCondition::toS(StrBuf *to) const {
			*to << expr;
		}


		// Create suitable conditions.
		Condition *STORM_FN createCondition(Expr *expr) {
			Value result = expr->result().type().asRef(false);

			if (result == Value(StormInfo<Bool>::type(expr->engine()))) {
				return new (expr) BoolCondition(expr);
			} else if (isMaybe(result)) {
				return new (expr) WeakMaybeCast(expr);
			} else {
				Str *msg = TO_S(expr->engine(), S("You can not use the type ")
								<< result << S(" as a condition."));
				throw new (expr) SyntaxError(expr->pos, msg);
			}
		}


		/**
		 * CondSuccess.
		 */

		CondSuccess::CondSuccess(SrcPos pos, Block *parent, Condition *cond) : Block(pos, parent), cond(cond) {
			if (LocalVar *var = cond->result())
				add(var);
		}

		void CondSuccess::set(Expr *e) {
			if (expr)
				throw new (e) UsageError(S("Cannot call CondSuccess::set multiple times!"));
			expr = e;
		}

		ExprResult CondSuccess::result() {
			if (expr)
				return expr->result();
			else
				return ExprResult();
		}

		void CondSuccess::blockCode(CodeGen *state, CodeResult *to) {
			// Initialize new variable if required:
			cond->trueCode(state);

			// Then we can execute the contained expression!
			if (expr)
				expr->code(state, to);
		}

		Int CondSuccess::castPenalty(Value to) {
			if (expr)
				return expr->castPenalty(to);
			else
				return -1;
		}

		void CondSuccess::toS(StrBuf *to) const {
			if (expr)
				*to << expr;
		}

	}
}
