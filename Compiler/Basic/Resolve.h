#pragma once
#include "Expr.h"
#include "Block.h"
#include "Actuals.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Custom name part used to traverse the named hierarchy and find elements that possibly
		 * need a 'this' pointer.
		 *
		 * To perform all name resolution in a single traversal of the name tree, the logic
		 * implemented here is as follows:
		 *
		 * 1. See if the raw name (i.e. without params) matches a type. If that is the case, match
		 *    the type if there is a suitable constructor there (so that we can later match a constructor call).
		 * 2. See if the name with parameters matches an entity. In that case, use that name.
		 * 3. See if the name with a this pointer matches an enity. If so, use that.
		 *
		 * In the implementation, we lump the two first together as a single pass.
		 *
		 * Note: We dont show the implicit this parameter, since some lookups will look at the first
		 * parameter to search in different parts of the name tree.
		 */
		class BSResolvePart : public BSNamePart {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSResolvePart(Str *name, SrcPos pos, Actuals *params, MAYBE(Expr *) thisExpr);

			// Check if an item matches in the first step. We modify the lookup a bit here to check
			// for type constructors as well.
			virtual Int STORM_FN matches(Named *candidate, Scope source) const;

			// Provide the next part if we need it.
			virtual MAYBE(SimplePart *) STORM_FN nextOption() const;

		private:
			// Part used for the second option.
			MAYBE(SimplePart *) secondPass;

			// Lookup used to match constructors (cache object so we don't need memory allocations).
			BSNamePart *ctorMatcher;
		};


		/**
		 * Unresolved named expression returned from 'namedExpr' in case a name is not found. This
		 * is useful since parts of the system (such as the assignment operator when using setters)
		 * need to inspect and modify a possibly incorrect name.
		 *
		 * Accessing any of the member functions that would require a valid name will throw an
		 * appropriate exception. This basically means that the error 'namedExpr' would throw is
		 * delayed until another class tries to access the result rather than being thrown immediately.
		 */
		class UnresolvedName : public Expr {
			STORM_CLASS;
		public:
			// Create. Takes the same parameters as the internal 'findTarget' function so that the
			// same query can be repeated later on.
			UnresolvedName(Block *block, SimpleName *name, SrcPos pos, Actuals *params, Bool useThis);

			// Block.
			Block *block;

			// Name.
			SimpleName *name;

			// Actual parameters.
			Actuals *params;

			// Use the 'this' parameter?
			Bool useThis;

			// Retry with different parameters.
			Expr *retry(Actuals *params) const;

			// Result type.
			virtual ExprResult STORM_FN result();

			// Generate code.
			virtual void STORM_FN code(CodeGen *s, CodeResult *to);

		protected:
			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		private:
			// Throw the error.
			void error() const;
		};


		// Find out what the named expression means, and create proper object.
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, SrcPos pos, Str *name, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, SrcName *name, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, SrcPos pos, Name *name, Actuals *params);

		// Special case of above, used when we find an expression like a.b(...). 'first' is inserted
		// into the beginning of 'params' and used. This method inhibits automatic insertion of 'this'.
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Expr *first, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, syntax::SStr *name, Expr *first);
		Expr *STORM_FN namedExpr(Block *block, SrcPos pos, Str *name, Expr *first, Actuals *params);
		Expr *STORM_FN namedExpr(Block *block, SrcPos pos, Str *name, Expr *first);

	}
}
