#pragma once
#include "Expr.h"
#include "Var.h"
#include "Block.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * A condition used in the if-statement and the loop.
		 *
		 * A condition in Basic Storm can provide a bit more functionality than just an
		 * expression. Conditions may also provide a single variable that is to be visible only in
		 * the "true" case of the conditional statement. This is used to implement "weak casts",
		 * i.e. casts that may fail. For this reason, the condition provides a 'result' member in
		 * addition to code generation. The generated code always produces a boolean expression, and
		 * if 'result' returned something other than 'null', that variable is initialized if the
		 * condition was true.
		 */
		class Condition : public ObjectOn<Compiler> {
			STORM_ABSTRACT_CLASS;
		public:
			// Create.
			STORM_CTOR Condition();

			// Get a suitable position for this condition.
			virtual SrcPos STORM_FN pos();

			// Get the variable that will be created by this condition if it is successful.
			virtual MAYBE(LocalVar *) STORM_FN result();

			// Generate code for the condition. This will create and initialize the "result"
			// variable, if applicable. Will return a boolean value as the CodeResult value.
			virtual void STORM_FN code(CodeGen *state, CodeResult *ok) ABSTRACT;

			// Generate code to initialize 'result' in the true branch. It is possible to assume
			// that 'code' was called previously.
			virtual void STORM_FN trueCode(CodeGen *state);
		};


		/**
		 * A regular boolean condition.
		 */
		class BoolCondition : public Condition {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BoolCondition(Expr *expr);

			// Get a suitable position for this condition.
			virtual SrcPos STORM_FN pos();

			// Result variable.
			virtual MAYBE(LocalVar *) STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *ok);

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// The expression we're evaluating.
			Expr *expr;
		};


		// Create a suitable condition for an expression.
		// TODO: Make this extensible somehow!
		Condition *STORM_FN createCondition(Expr *expr);



		/**
		 * A block used to encapsulate the additional variable created by a successful weak cast.
		 *
		 * It also ensures to call the "trueCode" inside a condition as necessary.
		 */
		class CondSuccess : public Block {
			STORM_CLASS;
		public:
			STORM_CTOR CondSuccess(SrcPos pos, Block *parent, Condition *cond);

			// Set the single contained expression. Throws if used more than once.
			void STORM_FN set(Expr *expr);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code generation.
			virtual void STORM_FN blockCode(CodeGen *state, CodeResult *to);

			// Auto casting.
			virtual Int STORM_FN castPenalty(Value to);

			// Anythin set?
			Bool STORM_FN any() const { return expr != null; }
			Bool STORM_FN empty() const { return expr == null; }

		protected:
			// To string.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Associated condition.
			Condition *cond;

			// Contained expression.
			MAYBE(Expr *) expr;
		};


	}
}
