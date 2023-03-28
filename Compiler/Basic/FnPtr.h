#pragma once
#include "Expr.h"
#include "Block.h"
#include "Compiler/Function.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Create a function pointer.
		 */
		class FnPtr : public Expr {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR FnPtr(Block *block, SrcName *name, Array<SrcName *> *formal);
			STORM_CTOR FnPtr(Block *block, Expr *dot, syntax::SStr *name, Array<SrcName *> *formal);

			// Create, deduce the parameters from context if possible.
			STORM_CTOR FnPtr(Block *block, SrcName *name);
			STORM_CTOR FnPtr(Block *block, Expr *dot, syntax::SStr *name);

			// Create, assuming we already know the exact function we want to get a pointer for.
			STORM_CTOR FnPtr(Function *target, SrcPos pos);
			STORM_CTOR FnPtr(Expr *dot, Function *target, SrcPos pos);

			// Result.
			virtual ExprResult STORM_FN result();

			// Code.
			virtual void STORM_FN code(CodeGen *state, CodeResult *r);

			// Cast penalty. Used to deduce parameter types for the function if none were provided.
			virtual Int STORM_FN castPenalty(Value to);

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Anything before the dot?
			MAYBE(Expr *) dotExpr;

			// Type of function pointer to return.
			Value ptrType;

			// Found function.
			MAYBE(Function *) target;

			// If 'ptrType' is 'void' and 'target' is 'null', then these two members are valid, and
			// we shall use them to deduce the parameters of the function later.
			MAYBE(Block *) parent;
			MAYBE(SimpleName *) name;

			// Initialize 'target' and 'type'.
			void findTarget(const Scope &scope, SrcName *name, Array<SrcName *> *formal, MAYBE(Expr *) dot);

			// Find an acceptable function for the result type 't'. Returns 'null' if none is
			// found. May modify "t" to account for wrapped maybes, etc.
			MAYBE(Function *) acceptableFn(Value &t);

			// Generate code.
			void code(CodeGen *state, CodeResult *r, Value type, Function *target);
		};

	}
}
