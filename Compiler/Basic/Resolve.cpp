#include "stdafx.h"
#include "Resolve.h"
#include "Named.h"
#include "Core/Join.h"
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


		// Helper to create the actual type, given something found. If '!useLookup', then we will not use the lookup
		// of the function or variable (ie use vtables).
		// 'first' is assumed to have been added implicitly.
		static Expr *findTarget(Scope scope, Named *n, LocalVar *first, Actuals *actual, const SrcPos &pos, bool useLookup) {
			if (!n)
				return null;

			if (*n->name == Type::CTOR)
				throw new (n) SyntaxError(pos, S("Can not call a constructor by using __ctor. Use Type() instead."));
			if (*n->name == Type::DTOR)
				throw new (n) SyntaxError(pos, S("Manual invocations of destructors are forbidden."));

			if (Function *f = as<Function>(n)) {
				if (first)
					actual = actual->withFirst(new (first) LocalVarAccess(pos, first));
				return new (n) FnCall(pos, scope, f, actual, useLookup);
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
				// If it is not a proper this-variable, don't allow using "super".
				if (!thisVar->thisVariable())
					throw new (block) SyntaxError(pos, S("Can not call 'super' outside of member functions."));

				SimpleName *part = name->from(1);

				// It is something in the super type!
				Type *super = thisVar->result.type->super();
				if (!super) {
					Str *msg = TO_S(block->engine(), S("No super type for ") << thisVar->result
									<< S(", can not use 'super' here."));
					throw new (block) SyntaxError(pos, msg);
				}

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
			try {
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

				StrBuf *msg = new (block) StrBuf();
				if (Type *t = as<Type>(n)) {
					*msg << n->identifier() << S(" refers to a type, but no suitable constructor was found.\n");
					*msg << S("  Available constructors:\n");
					for (NameSet::Iter b = t->begin(), e = t->end(); b != e; b++) {
						Named *v = b.v();
						if (*v->name == Type::CTOR)
							*msg << S("  ") << v << S("\n");
					}
				} else {
					*msg << n->identifier() << S(" is a ") << runtime::typeOf(n)->identifier()
						 << S(". Only functions, variables and constructors can be used in this way.");
				}

				throw new (block) TypeError(pos, msg->toS());

			} catch (CodeError *error) {
				// Add position information if it is available.
				if (error->pos.empty())
					error->pos = pos;
				throw;
			}
		}

		/**
		 * New implementation:
		 */

		BSResolvePart::BSResolvePart(Str *name, SrcPos pos, Actuals *params, MAYBE(LocalVar *) thisVar)
			: BSNamePart(name, pos, params), thisVar(thisVar) {}

		Int BSResolvePart::matches(Named *candidate, Scope source) const {
			// #1: If it is a type without parameters, match it since we want to match its constructors.
			if (candidate->params->empty()) {
				if (as<Type>(candidate)) {
					return 0;
				}
			}

			// #2: ...

			// #3: ...

			return 0;
		}

		// Find a constructor.
		static MAYBE(Expr *) findCtor(Scope scope, Type *t, Actuals *actual, const SrcPos &pos) {
			BSNamePart *part = new (t) BSNamePart(Type::CTOR, pos, actual);
			part->insert(thisPtr(t));

			Function *ctor = as<Function>(t->find(part, scope));
			if (!ctor)
				return null;

			return new (t) CtorCall(pos, scope, ctor, actual);
		}

		// Find a constructor. Throw otherwise.
		static Expr *findCtorSafe(Scope scope, Type *t, Actuals *actual, const SrcPos &pos) {
			if (Expr *e = findCtor(scope, t, actual, pos))
				return e;

			*msg << n->identifier() << S(" refers to a type, but no suitable constructor was found.\n");
			*msg << S("  Available constructors:\n");
			for (NameSet::Iter b = t->begin(), e = t->end(); b != e; b++) {
				Named *v = b.v();
				if (*v->name == Type::CTOR)
					*msg << S("  ") << v << S("\n");
			}
			throw new (t) TypeError(pos, msg->toS());
		}

		// Find the this variable if it exists.
		static MAYBE(LocalVar *) findThis(Block *block) {
			return block->variable(new (block) SimplePart(S("this")));
		}

		// Find a constructor call.
		static Expr *findTargetCtor(Block *block, SimpleName *name, const SrcPos &pos, Actuals *params, Bool useThis) {
			Scope scope = block->scope;
			Named *n = scope.find(name);
			if (!name)
				return new (block) UnresolvedName(block, name, pos, params, useThis);

			if (Type *t = as<Type>(n))
				return findCtorSafe(scope, t, params, pos);

			StrBuf *msg = new (this) StrBuf();
			*msg << n->identifier() << S(" refers to a ") << runtime::typeOf(n)->identifier()
				 << S(". However, since you have specified explicit template parameters, ")
				 << S("it has to refer to a type.");
			throw new (block) TypeError(pos, msg->toS());
		}

		// Find the what the meaning of 'name' is in the given context. Returns a suitable
		// expression if 'name' can be resolved, or an 'UnresolvedName' to indicate failure. This is
		// so that implementations can detect failure without relying on exceptions. The
		// 'UnresolvedName' throws whenever it is accessed. If 'useThis' is true, then we will
		// attempt to insert a 'this' pointer if it exists, and if it is required by the situation.
		static Expr *findTarget(Block *block, SimpleName *name, const SrcPos &pos, Actuals *params, bool useThis) {
			assert(name->any(), L"Cannot resolve an empty name.");

			// It is important that we traverse the scope hierarcy *once* here. Otherwise, we might
			// accidentally give items close to the root of the name tree precedence over items
			// close to the leaves. This often leads to confusing results.
			Scope scope = block->scope;
			SimplePart *last = name->last();

			if (last->params->any()) {
				// If we have parameters, we need to look for a template type and find a constructor
				// inside that type. That is the only option.
				return findTargetCtor(block, name, pos, params, useThis);
			}

			// Otherwise, create a BSResolvePart that we use to traverse the name tree. For this, we
			// need to examine if we should use a 'this' pointer, and if it exists.
			LocalVar *thisVar = useThis ? findThis() : null;
			if (thisVar && isSuperName(name)) {
				if (!thisVar->thisVariable())
					throw new (block) SyntaxError(pos, S("It is not possible to use 'super' outside of member functions."));

				// This is a "super" expression. Handle it.
				assert(false, L"TODO!");
			}

			BSResolvePart *part = new (this) BSResolvePart(last->name, pos, params, thisType);
			name->last() = part;

			// Traverse the name tree.
			Named *found = scope.find(name);
			if (!found)
				return new (this) UnresolvedName(block, name, pos, params, useThis);

			// Translate it into a suitable expression.
			if (Expr *expr = toExpression(scope, found, thisVar, actual, pos, true))
				return expr;

			// Error message if it is not supported.
			StrBuf *msg = new (this) StrBuf();
			*msg << found->identifier() S(" refers to a ") << runtime::typeOf(found)->identifier()
				 << S(". This type is not natively supported in Basic Storm, and can therefore not ")
				 << S("be used without custom syntax. Only functions, variables, and constructors ")
				 << S("can be used in this manner.");
			throw new (block) TypeError(pos, msg->toS());
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
				throw new (block) SyntaxError(pos, TO_S(block, S("Could not resolve parameters in ") << name));

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
			throw new (this) SyntaxError(pos, TO_S(engine(), S("Can not find ") << name << S(".")));
		}

	}
}
