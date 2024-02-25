#pragma once
#include "Block.h"
#include "Breakable.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * For loop (basic). The start expression should be placed in a block outside the for loop.
		 *
		 * TODO: Maybe use conditional expressions as the test here as well?
		 */
		class For : public Breakable {
			STORM_CLASS;
		public:
			STORM_CTOR For(SrcPos pos, Block *parent);

			// Test expression.
			Expr *testExpr;
			void STORM_FN test(Expr *e);

			// Update expression.
			Expr *updateExpr;
			void STORM_FN update(Expr *e);

			// Body.
			Expr *bodyExpr;
			void STORM_FN body(Expr *e);

			// Result (always void).
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(CodeGen *s, CodeResult *to, code::Block block);

			// Break- and continue management.
			virtual void STORM_FN willBreak();
			virtual void STORM_FN willContinue();

			virtual Breakable::To STORM_FN breakTo();
			virtual Breakable::To STORM_FN continueTo();

		protected:
			// ToS.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			code::Block block;
			code::Label brk;
			code::Label cont;
		};

	}
}
