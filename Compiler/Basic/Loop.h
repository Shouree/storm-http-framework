#pragma once
#include "Block.h"
#include "Breakable.h"
#include "WeakCast.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * The loop in BS. Either:
		 * do { }
		 * do { } while (x);
		 * do { } while (x) { }
		 * while (x) { }
		 */
		class Loop : public Breakable {
			STORM_CLASS;
		public:
			STORM_CTOR Loop(SrcPos pos, Block *parent);

			// Set the condition.
			void STORM_FN cond(Condition *cond);

			// Helpers for common tasks.
			void STORM_FN condExpr(Expr *expr);

			// Get the condition. Throw on failure.
			Condition *STORM_FN cond();

			// Do content (if any). Will adopt any variables in the block 'e' to make scoping
			// correct in 'while', and allow local variables inside the 'do' part of the loop to be
			// used in the condition.
			void STORM_FN doBody(Expr *e);

			// Set the body for the while expression.
			void STORM_FN whileBody(Expr *s);

			// Get the scope for the wile body.
			CondSuccess *STORM_FN whileBlock();

			// Result (always void or 'no return').
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Break- and continue management.
			virtual void STORM_FN willBreak();
			virtual void STORM_FN willContinue();

			virtual Breakable::To STORM_FN breakTo();
			virtual Breakable::To STORM_FN continueTo();

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Condition (if any).
			MAYBE(Condition *) condition;

			// Do-content.
			MAYBE(Expr *) doExpr;

			// While content.
			MAYBE(CondSuccess *) whileExpr;

			// Did we find any break statements.
			Bool anyBreak;

			// During codegen: where to break and continue to/from.
			code::Block breakBlock;
			code::Block continueBlock;
			code::Label before;
			code::Label after;

			// Code generation.
			void code(CodeGen *outer, CodeGen *inner, CodeResult *r);
		};


	}
}
