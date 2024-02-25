#pragma once
#include "Block.h"
#include "Condition.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Implements the unless (<cast>) return X; ... syntax.
		 */
		class Unless : public Block {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Unless(Block *block, Condition *cond);

			// Condition.
			Condition *cond;

			// Block to execute if the cast succeeds. Also contains additional variables that are
			// visible in the case that the cast succeeded.
			CondSuccess *successBlock;

			// Statement to be executed if the cast fails.
			MAYBE(Expr *) failStmt;

			// Set the fail statement. Helper used from the grammar.
			void STORM_FN fail(Expr *expr);

			// Set the contents of the success block. Helper used from the grammar.
			void STORM_FN success(Expr *expr);

			// Result.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};


	}
}
