#include "stdafx.h"
#include "Loop.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Loop::Loop(SrcPos pos, Block *parent) : Breakable(pos, parent), anyBreak(false) {}

		void Loop::cond(Condition *cond) {
			condition = cond;
		}

		Condition *Loop::cond() {
			if (!condition)
				throw new (this) UsageError(S("Must call 'cond' before creating a while body!"));
			return condition;
		}

		void Loop::condExpr(Expr *e) {
			cond(new (this) BoolCondition(e));
		}

		void Loop::doBody(Expr *e) {
			doExpr = e;
			if (Block *b = as<Block>(e))
				liftVars(b);
		}

		void Loop::whileBody(CondSuccess *e) {
			whileExpr = e;
		}

		CondSuccess *Loop::createWhileBody() {
			if (!condition)
				throw new (this) UsageError(S("Must call 'cond' before creating a while body!"));
			whileExpr = new (this) CondSuccess(pos, this, condition);
			return whileExpr;
		}

		ExprResult Loop::result() {
			if (condition || anyBreak) {
				// No reliable value, the last expression in the do part could be used.
				return ExprResult();
			} else {
				// If we don't have a condition, we never return.
				return noReturn();
			}
		}

		void Loop::code(CodeGen *s, CodeResult *r) {
			using namespace code;

			// Outer state where we store our control variable!
			CodeGen *outer = s->child();
			*s->l << begin(outer->block);

			// Inner state, which represents the actual block we're in.
			CodeGen *inner = outer->child();

			// Initialize variables in the scope.
			initVariables(inner);


			code(outer, inner, r);

			*s->l << end(outer->block);
		}

		void Loop::code(CodeGen *outerState, CodeGen *innerState, CodeResult *r) {
			using namespace code;

			Listing *l = innerState->l;
			breakBlock = innerState->block;
			continueBlock = outerState->block;
			before = l->label();
			after = l->label();

			*l << before;
			*l << begin(innerState->block);

			// Things in the do-part.
			if (doExpr) {
				CodeResult *doResult = new (this) CodeResult();
				doExpr->code(innerState, doResult);
			}

			// Check the condition.
			if (condition) {
				CodeResult *condResult = new (this) CodeResult(Value(StormInfo<Bool>::type(engine())), innerState->block);
				condition->code(innerState, condResult);

				code::Var c = condResult->location(innerState);
				*l << cmp(c, byteConst(0));
				*l << jmp(after, ifEqual);
			}

			// Things in the while-part.
			if (whileExpr) {
				Label after = innerState->l->label();

				CodeResult *whileResult = new (this) CodeResult();
				whileExpr->code(innerState, whileResult);

				*l << after;
			}

			// Jump back to the start.
			*l << jmpBlock(before, outerState->block);

			*l << after;
			*l << end(innerState->block);
		}

		void Loop::willBreak() {
			anyBreak = true;
		}

		void Loop::willContinue() {}

		Breakable::To Loop::breakTo() {
			return Breakable::To(after, breakBlock);
		}

		Breakable::To Loop::continueTo() {
			return Breakable::To(before, continueBlock);
		}

		void Loop::toS(StrBuf *to) const {
			if (doExpr) {
				*to << S("do ") << doExpr;
			}

			if (condition) {
				if (doExpr)
					*to << S(" ");
				*to << S("while (") << condition << S(") ");
			}

			if (whileExpr) {
				*to << whileExpr;
			}
		}

	}
}
