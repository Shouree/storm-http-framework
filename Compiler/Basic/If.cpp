#include "stdafx.h"
#include "If.h"
#include "Cast.h"
#include "Named.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {

		If::If(Block *parent, Condition *cond) : Block(cond->pos(), parent), condition(cond) {
			successBlock = new (this) CondSuccess(pos, this, condition);
		}

		If::If(Block *parent, Expr *expr) : Block(expr->pos, parent) {
			condition = new (this) BoolCondition(expr);
			successBlock = new (this) CondSuccess(pos, this, condition);
		}

		void If::success(Expr *expr) {
			successBlock->set(expr);
		}

		void If::fail(Expr *expr) {
			failStmt = expr;
		}

		ExprResult If::result() {
			if (failStmt) {
				return common(successBlock, failStmt, scope);
			} else {
				return ExprResult();
			}
		}

		void If::blockCode(CodeGen *state, CodeResult *r) {
			using namespace code;

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
				Expr *t = expectCastTo(successBlock, rType, scope);
				t->code(state, r->split(state));
			}

			if (failStmt) {
				*state->l << jmp(lblDone);
			}

			*state->l << lblElse;

			if (failStmt) {
				// False branch:
				Expr *f = expectCastTo(failStmt, rType, scope);
				f->code(state, r->split(state));

				*state->l << lblDone;
			}

			// Notify that all branches have created the value.
			r->created(state);
		}

		void If::toS(StrBuf *to) const {
			*to << S("if (") << condition << S(") ");
			*to << successBlock;
			if (failStmt) {
				*to << S(" else ");
				*to << failStmt;
			}
		}

	}
}
