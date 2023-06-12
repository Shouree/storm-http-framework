#include "stdafx.h"
#include "Unless.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Unless::Unless(Block *parent, Condition *cond) : Block(cond->pos(), parent), cond(cond) {
			successRoot = new (this) CondSuccess(pos, this, cond);
			successBlock = new (this) ExprBlock(pos, successRoot);
			successRoot->set(successBlock);
		}

		void Unless::fail(Expr *expr) {
			if (expr->result() != noReturn())
				throw new (this) SyntaxError(pos, S("The block in 'until' must always do a 'return', 'throw', 'break' or 'continue'.!"));
			failStmt = expr;
		}

		void Unless::success(Expr *expr) {
			successBlock->add(expr);
		}

		ExprResult Unless::result() {
			return successRoot->result();
		}

		void Unless::blockCode(CodeGen *state, CodeResult *r) {
			using namespace code;

			CodeResult *ok = new (this) CodeResult(Value(StormInfo<Bool>::type(engine())), state->block);
			cond->code(state, ok);

			Label skipLbl = state->l->label();

			*state->l << cmp(ok->location(state), byteConst(0));
			*state->l << jmp(skipLbl, ifNotEqual);

			CodeResult *failResult = CREATE(CodeResult, this);
			failStmt->code(state, failResult);
			// We have verified that 'failStmt' never returns, no worry about extra jumps!

			*state->l << skipLbl;

			successRoot->code(state, r);
		}

		void Unless::toS(StrBuf *to) const {
			*to << S("unless (") << cond << S(") ") << failStmt << S(";\n") << successBlock;
		}


	}
}
