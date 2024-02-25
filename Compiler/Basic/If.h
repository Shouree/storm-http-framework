#pragma once
#include "Block.h"
#include "WeakCast.h"
#include "Condition.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * If-statement.
		 *
		 * Note that the if-statement provides the "successBlock" member that is a special scope
		 * used for the true branch of the if-statement.
		 */
		class If : public Block {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR If(Block *parent, Condition *cond);

			// Shorthand for creating a condition.
			STORM_CTOR If(Block *parent, Expr *expr);

			// The condition itself.
			Condition *condition;

			// Success branch. Created by the constructor, use to properly scope variables only
			// available in the successful branch.
			CondSuccess *successBlock;

			// Failure branch, if present.
			MAYBE(Expr *) failStmt;

			// Helpers to set the success and fail branches.
			void STORM_FN success(Expr *expr);
			void STORM_FN fail(Expr *expr);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *r);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
		};

	}
}
