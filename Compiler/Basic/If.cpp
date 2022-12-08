#include "stdafx.h"
#include "If.h"
#include "Cast.h"
#include "Named.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {

		If::If(Block *parent, Condition *cond) : Block(cond->pos(), parent), condition(cond) {}

		If::If(Block *parent, Expr *expr) : Block(expr->pos, parent) {
			condition = new (this) BoolCondition(expr);
		}

		void If::trueBranch(CondSuccess *e) {
			trueCode = e;
		}

		void If::falseBranch(Expr *e) {
			falseCode = e;
		}

		ExprResult If::result() {
			if (falseCode && trueCode) {
				return common(trueCode, falseCode, scope);
			} else {
				return ExprResult();
			}
		}

		void If::blockCode(CodeGen *state, CodeResult *r) {
			using namespace code;

			if (!trueCode)
				throw new (this) UsageError(S("Must set the true branch in an if-statement!"));

			Value condType(StormInfo<Bool>::type(engine()));
			CodeResult *condResult = new (this) CodeResult(condType, state->block);
			condition->code(state, condResult);

			Label lblElse = state->l->label();
			Label lblDone = state->l->label();

			code::Var c = condResult->location(state);
			*state->l << cmp(c, byteConst(0));
			*state->l << jmp(lblElse, ifEqual);

			Value rType = result().type();

			// True branch:
			{
				Expr *t = expectCastTo(trueCode, rType, scope);
				t->code(state, r->split(state));
			}

			if (falseCode) {
				*state->l << jmp(lblDone);
			}

			*state->l << lblElse;

			if (falseCode) {
				// False branch:
				Expr *f = expectCastTo(falseCode, rType, scope);
				f->code(state, r->split(state));

				*state->l << lblDone;
			}

			// Notify that all branches have created the value.
			r->created(state);
		}

		void If::toS(StrBuf *to) const {
			*to << S("if (") << condition << S(") ");
			*to << trueCode;
			if (falseCode) {
				*to << S(" else ");
				*to << falseCode;
			}
		}

	}
}
