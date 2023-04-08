#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Syntax/SStr.h"
#include "Compiler/Name.h"
#include "Compiler/Scope.h"
#include "Expr.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Actual parameters to a function.
		 */
		class Actuals : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			STORM_CTOR Actuals();
			STORM_CTOR Actuals(Expr *expr);
			Actuals(const Actuals &o);

			// Parameters.
			Array<Expr *> *expressions;

			// Compute all types.
			Array<Value> *STORM_FN values();

			// Generate the code to get one parameter. Returns where it is stored.
			// 'type' may differ slightly from 'expressions->at(id)->result()'.
			code::Operand STORM_FN code(Nat id, CodeGen *s, Value type, Scope scope);

			// Empty?
			inline Bool STORM_FN empty() { return expressions->empty(); }
			inline Bool STORM_FN any() { return expressions->any(); }

			// Count?
			inline Nat STORM_FN count() { return expressions->count(); }

			// Add a parameter.
			void STORM_FN add(Expr *expr);

			// Add a parameter to the beginning.
			void STORM_FN addFirst(Expr *expr);

			// Add a first parameter to a copy of this object.
			Actuals *STORM_FN withFirst(Expr *expr) const;

			// Is this call using a "this"-parameter as the first parameter?
			Bool STORM_FN hasThisFirst() const;

		protected:
			virtual void STORM_FN toS(StrBuf *to) const;
		};

		/**
		 * Extension to the name part class that takes care of the automatic casts supported by
		 * Basic Storm. It delegates these checks to the BSAutocast-header.
		 *
		 * Note that Actuals needs to be updated (to some degree) to make some casts work.
		 */
		class BSNamePart : public SimplePart {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BSNamePart(syntax::SStr *name, Actuals *params);
			STORM_CTOR BSNamePart(Str *name, SrcPos pos, Actuals *params);
			BSNamePart(const wchar *name, SrcPos pos, Actuals *params);

			// Insert an expression as the first parameter (used for this pointers).
			void STORM_FN insert(Expr *first);
			void STORM_FN insert(Expr *first, Nat at);

			// Insert a type as the first parameter (used for this pointers).
			void STORM_FN insert(Value first);
			void STORM_FN insert(Value first, Nat at);

			// Alter an expression.
			void STORM_FN alter(Nat id, Value to);

			// Match the first parameter strictly, ie. do not allow automatic type casts of the
			// first parameter.
			void STORM_FN strictFirst();

			// Set the first parameter as having been specified explicitly.
			void STORM_FN explicitFirst();

			// Was the first parameter explicit?
			Bool STORM_FN scopeParam(Nat id) const;

			// Create a copy of this object, but inserting a new value as the first parameter.
			BSNamePart *STORM_FN withFirst(Value val) const;

			// Matches?
			virtual Int STORM_FN matches(Named *candidate, Scope source) const;

		protected:
			// Check a single parameter. Used to implement a custom 'matches' in derived classes.
			Int checkParam(Value formal, Nat index, Bool first, NamedFlags flags, Scope context) const;
			Int checkParam(Value formal, Expr *actual, Bool first, NamedFlags flags, Scope context) const;

			// Position of this part.
			SrcPos pos;

		private:
			// Original expressions. (may contain null).
			Array<Expr *> *exprs;

			// Strict matching of the 'this' parameter.
			Bool strictThis;

			// Explicit first parameter.
			Bool expFirst;

			BSNamePart(Str *name, SrcPos pos, Array<Expr *> *exprs);
		};

		// Helper to create a Name with one BSNamePart inside of it.
		Name *STORM_FN bsName(syntax::SStr *name, Actuals *params) ON(Compiler);
		Name *STORM_FN bsName(Str *name, SrcPos pos, Actuals *params) ON(Compiler);

	}
}
