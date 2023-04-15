#pragma once
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class FnBody;

		/**
		 * Return statement.
		 */
		class Return : public Expr {
			STORM_CLASS;
		public:
			// Return nothing.
			STORM_CTOR Return(SrcPos pos, Block *block);

			// Return a value.
			STORM_CTOR Return(SrcPos pos, Block *block, Expr *expr);

			// Value to return.
			MAYBE(Expr *) expr;

			// Result (never returns).
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Function body we are returning from.
			FnBody *body;

			// Check type.
			void checkType();

			// Find the surrounding FnBody.
			FnBody *findParentFn(SrcPos pos, Block *block);

			// Code generation.
			void voidCode(CodeGen *state);
			void valueCode(CodeGen *state);
		};

	}
}
