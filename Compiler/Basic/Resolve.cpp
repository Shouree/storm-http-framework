#include "stdafx.h"
#include "Resolve.h"
#include "Named.h"
#include "Compiler/Engine.h"
#include "Compiler/Type.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		/**
		 * Look up a proper action from a name and a set of parameters.
		 */
		static MAYBE(Expr *) findCtor(Scope scope, Type *t, Actuals *actual, const SrcPos &pos);
		static Expr *findTarget(Scope scope, Named *n, LocalVar *first, Actuals *actual, const SrcPos &pos, bool useLookup);
		static Expr *findTargetThis(Block *block, SimpleName *name,
									Actuals *params, const SrcPos &pos,
									Named *&candidate);
		static Expr *findTarget(Block *block, SimpleName *name,
								const SrcPos &pos, Actuals *params,
								bool useThis);

		// Find a constructor.
		static MAYBE(Expr *) findCtor(Scope scope, Type *t, Actuals *actual, const SrcPos &pos) {
			BSNamePart *part = new (t) BSNamePart(Type::CTOR, pos, actual);
			part->insert(thisPtr(t));

			Function *ctor = as<Function>(t->find(part, scope));
			if (!ctor)
				return null;
			// throw SyntaxError(pos, L"No constructor " + ::toS(t->identifier()) + L"." + ::toS(part) + L")");

			return new (t) CtorCall(pos, scope, ctor, actual);
		}


		// Helper to create the actual type, given something found. If '!useLookup', then we will not use the lookup
		// of the function or variable (ie use vtables).
		static Expr *findTarget(Scope scope, Named *n, LocalVar *first, Actuals *actual, const SrcPos &pos, bool useLookup) {
			if (!n)
				return null;

			if (*n->name == Type::CTOR)
				throw SyntaxError(pos, L"Can not call a constructor by using __ctor. Use Type() instead.");
			if (*n->name == Type::DTOR)
				throw SyntaxError(pos, L"Manual invocations of destructors are forbidden.");

			if (Function *f = as<Function>(n)) {
				if (first)
					actual = actual->withFirst(new (first) LocalVarAccess(pos, first));
				return new (n) FnCall(pos, scope, f, actual, useLookup, first ? true : false);
			}

			if (LocalVar *v = as<LocalVar>(n)) {
				assert(!first);
				return new (n) LocalVarAccess(pos, v);
			}

			if (MemberVar *v = as<MemberVar>(n)) {
				if (first)
					return new (n) MemberVarAccess(pos, new (first) LocalVarAccess(pos, first), v, true);
				else
					return new (n) MemberVarAccess(pos, actual->expressions->at(0), v, false);
			}

			if (NamedThread *v = as<NamedThread>(n)) {
				assert(!first);
				return new (n) NamedThreadAccess(pos, v);
			}

			if (GlobalVar *v = as<GlobalVar>(n)) {
				assert(!first);
				return new (n) GlobalVarAccess(pos, v);
			}

			return null;
		}

		static bool isSuperName(SimpleName *name) {
			if (name->count() != 2)
				return false;

			SimplePart *p = name->at(0);
			if (*p->name != S("super"))
				return false;
			return p->params->empty();
		}

		// Find a target assuming we should use the this-pointer.
		static Expr *findTargetThis(Block *block, SimpleName *name,
											Actuals *params, const SrcPos &pos,
											Named *&candidate) {
			const Scope &scope = block->scope;

			SimplePart *thisPart = new (block) SimplePart(new (block) Str(S("this")));
			LocalVar *thisVar = block->variable(thisPart);
			if (!thisVar)
				return null;

			BSNamePart *lastPart = new (name) BSNamePart(name->last()->name, pos, params);
			lastPart->insert(thisVar->result);
			bool useLookup = true;

			if (isSuperName(name)) {
				SimpleName *part = name->from(1);
				// It is something in the super type!
				Type *super = thisVar->result.type->super();
				if (!super)
					throw SyntaxError(pos, L"No super type for " + ::toS(thisVar->result) + L", can not use 'super' here.");

				part->last() = lastPart;
				candidate = storm::find(block->scope, super, part);
				useLookup = false;
			} else {
				// May be anything.
				name->last() = lastPart;
				candidate = scope.find(name);
				useLookup = true;
			}

			Expr *e = findTarget(block->scope, candidate, thisVar, params, pos, useLookup);
			if (e)
				return e;

			return null;
		}

		// Find whatever is meant by the 'name' in this context. Return suitable expression. If
		// 'useThis' is true, a 'this' pointer may be inserted as the first parameter.
		static Expr *findTarget(Block *block, SimpleName *name, const SrcPos &pos, Actuals *params, bool useThis) {
			const Scope &scope = block->scope;

			// Type ctors and local variables have priority.
			{
				Named *n = scope.find(name);
				if (Type *t = as<Type>(n)) {
					// If we find a suitable constructor, go for it. Otherwise, continue.
					if (Expr *e = findCtor(block->scope, t, params, pos))
						return e;
				} else if (as<LocalVar>(n) != null && params->empty()) {
					return findTarget(block->scope, n, null, params, pos, false);
				}
			}

			// If we have a this-pointer, try to use it!
			Named *candidate = null;
			if (useThis)
				if (Expr *e = findTargetThis(block, name, params, pos, candidate))
					return e;

			// Try without the this pointer.
			BSNamePart *last = new (name) BSNamePart(name->last()->name, pos, params);
			name->last() = last;
			Named *n = scope.find(name);

			if (Expr *e = findTarget(block->scope, n, null, params, pos, true))
				return e;

			if (!n && !candidate)
				// Delay throwing the error until later.
				return new (name) UnresolvedName(block, name, pos, params, useThis);

			if (!n)
				n = candidate;

			throw TypeError(pos, ::toS(n) + L" is a " + ::toS(runtime::typeOf(n)->identifier()) +
							L". Only functions, variables and constructors are supported.");
		}

		Expr *namedExpr(Block *block, syntax::SStr *name, Actuals *params) {
			return namedExpr(block, name->pos, name->v, params);
		}

		Expr *namedExpr(Block *block, SrcPos pos, Str *name, Actuals *params) {
			SimpleName *n = new (name) SimpleName(name);
			return findTarget(block, n, pos, params, true);
		}

		Expr *namedExpr(Block *block, SrcName *name, Actuals *params) {
			return namedExpr(block, name->pos, name, params);
		}

		Expr *namedExpr(Block *block, SrcPos pos, Name *name, Actuals *params) {
			SimpleName *simple = name->simplify(block->scope);
			if (!simple)
				throw SyntaxError(pos, L"Could not resolve parameters in " + ::toS(name));

			return findTarget(block, simple, pos, params, true);
		}

		Expr *namedExpr(Block *block, syntax::SStr *name, Expr *first, Actuals *params) {
			return namedExpr(block, name->pos, name->v, first, params);
		}

		Expr *namedExpr(Block *block, syntax::SStr *name, Expr *first) {
			return namedExpr(block, name->pos, name->v, first);
		}

		Expr *namedExpr(Block *block, SrcPos pos, Str *name, Expr *first, Actuals *params) {
			params = params->withFirst(first);
			SimpleName *n = new (name) SimpleName(name);
			return findTarget(block, n, pos, params, false);
		}

		Expr *namedExpr(Block *block, SrcPos pos, Str *name, Expr *first) {
			Actuals *params = new (block) Actuals();
			params->add(first);
			SimpleName *n = new (name) SimpleName(name);
			return findTarget(block, n, pos, params, false);
		}


		/**
		 * Unresolved name.
		 */

		UnresolvedName::UnresolvedName(Block *block, SimpleName *name, SrcPos pos, Actuals *params, Bool useThis) :
			Expr(pos), block(block), name(name), params(params), useThis(useThis) {}

		Expr *UnresolvedName::retry(Actuals *params) const {
			return findTarget(block, name, pos, params, useThis);
		}

		ExprResult UnresolvedName::result() {
			error();
			return ExprResult();
		}

		void UnresolvedName::code(CodeGen *s, CodeResult *r) {
			error();
		}

		void UnresolvedName::toS(StrBuf *to) const {
			*to << name << S("[invalid]");
		}

		void UnresolvedName::error() const {
			throw SyntaxError(pos, L"Can not find " + ::toS(name) + L".");
		}

	}
}
