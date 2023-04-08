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
		 * New implementation:
		 */

		BSResolvePart::BSResolvePart(Str *name, SrcPos pos, Actuals *params, MAYBE(Expr *) thisExpr)
			: BSNamePart(name, pos, params) {

			// If we have a thisExpr, create a BSNamePart that matches with the this pointer.
			if (thisExpr)
				secondPass = new (this) BSNamePart(name, pos, params->withFirst(thisExpr));
		}

		Int BSResolvePart::matches(Named *candidate, Scope source) const {
			// #1: If this is a type without parameters, see if it is a type that contains a
			// suitable constructor. If so, mark it as a candidate according to how well the
			// constructor fits.
			if (candidate->params->empty()) {
				if (Type *t = as<Type>(candidate)) {

					if (ctorMatcher) {
						ctorMatcher->alter(0, thisPtr(t));
					} else {
						ctorMatcher = this->withFirst(thisPtr(t));
						ctorMatcher->name = new (this) Str(Type::CTOR);
					}

					if (Named *ctor = t->find(ctorMatcher, source)) {
						// Compute penalty for this option.
						Int distance = 0;
						for (Nat i = 1; i < ctor->params->count(); i++) {
							Int d = checkParam(ctor->params->at(i), i - 1, false, ctor->flags, source);
							// This should not happen, but better safe than sorry.
							if (d < 0)
								return -1;
							distance += d;
						}
						return distance;
					}
				}
			}

			// If it was not a constructor, just call the logic inside BSResolvePart.
			return BSNamePart::matches(candidate, source);
		}

		MAYBE(SimplePart *) BSResolvePart::nextOption() const {
			return secondPass;
		}

		static bool isSuperName(SimpleName *name) {
			if (name->count() != 2)
				return false;

			SimplePart *p = name->at(0);
			if (*p->name != S("super"))
				return false;
			return p->params->empty();
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

			StrBuf *msg = new (t) StrBuf();
			*msg << t->identifier() << S(" refers to a type, but no suitable constructor was found.\n");
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
			if (!n)
				return new (block) UnresolvedName(block, name, pos, params, useThis);

			if (Type *t = as<Type>(n))
				return findCtorSafe(scope, t, params, pos);

			StrBuf *msg = new (block) StrBuf();
			*msg << n->identifier() << S(" refers to a ") << runtime::typeOf(n)->identifier()
				 << S(". However, since you have specified explicit template parameters, ")
				 << S("it has to refer to a type.");
			throw (new (block) TypeError(pos, msg->toS()))->saveTrace();
		}


		// Examine the type of a named object we found, and create a suitable expression.
		static Expr *toExpression(const Scope &scope,
								Named *found, Actuals *params,
								const SrcPos &pos,
								bool firstImplicit, bool useLookup) {

			if (*found->name == Type::CTOR) {
				StrBuf *msg = new (found) StrBuf();
				Named *parent = as<Named>(found->parent());
				Str *typeName = parent ? parent->name : new (found) Str(S("T"));
				*msg << S("It is not possible to call a constructor by using ")
					 << typeName << S(".__ctor(...). Use ")
					 << typeName << S("(...) instead.");
				throw new (found) SyntaxError(pos, msg->toS());
			}

			if (*found->name == Type::DTOR)
				throw new (found) SyntaxError(pos, S("Manual invocation of destructors is not possible."));

			if (Function *f = as<Function>(found))
				return new (found) FnCall(pos, scope, f, params, useLookup);

			if (LocalVar *v = as<LocalVar>(found))
				return new (found) LocalVarAccess(pos, v);

			if (MemberVar *v = as<MemberVar>(found))
				return new (found) MemberVarAccess(pos, params->expressions->at(0), v, firstImplicit);

			if (NamedThread *t = as<NamedThread>(found))
				return new (found) NamedThreadAccess(pos, t);

			if (GlobalVar *v = as<GlobalVar>(found))
				return new (found) GlobalVarAccess(pos, v);

			// Error message if it is not supported.
			StrBuf *msg = new (found) StrBuf();
			*msg << found->identifier() << S(" refers to a ") << runtime::typeOf(found)->identifier()
				 << S(". This type is not natively supported in Basic Storm, and can therefore not ")
				 << S("be used without custom syntax. Only functions, variables, and constructors ")
				 << S("can be used in this manner.");
			throw new (found) TypeError(pos, msg->toS());
		}

		// Find the meaning of an expression of the type "super(...)".
		static Expr *findTargetSuper(Block *block, SimpleName *name, const SrcPos &pos, Actuals *params, LocalVar *thisVar) {
			// Check if we can resolve the name in the context of the super class.
			Type *super = thisVar->result.type->super();
			if (!super) {
				Str *msg = TO_S(block, S("The type ") << thisVar->result << S(" does not have a super type."));
				throw new (block) SyntaxError(pos, msg);
			}

			params = params->withFirst(new (block) LocalVarAccess(pos, thisVar));
			SimpleName *noSuper = name->from(1);
			noSuper->last() = new (block) BSNamePart(name->last()->name, pos, params);

			Named *found = storm::find(block->scope, super, noSuper);
			if (!found)
				return new (block) UnresolvedName(block, name, pos, params, true);

			return toExpression(block->scope, found, params, pos, true, false);
		}

		// Find the what the meaning of 'name' is in the given context. Returns a suitable
		// expression if 'name' can be resolved, or an 'UnresolvedName' to indicate failure. This is
		// so that implementations can detect failure without relying on exceptions. The
		// 'UnresolvedName' throws whenever it is accessed. If 'useThis' is true, then we will
		// attempt to insert a 'this' pointer if it exists, and if it is required by the situation.
		static Expr *findTargetI(Block *block, SimpleName *name, const SrcPos &pos, Actuals *params, bool useThis) {
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
			LocalVar *thisVar = useThis ? findThis(block) : null;
			if (thisVar && isSuperName(name)) {
				if (!thisVar->thisVariable())
					throw new (block) SyntaxError(pos, S("It is not possible to use 'super' outside of member functions."));
				if (name->at(0)->params->any())
					throw new (block) SyntaxError(pos, S("It is not possible to provide template parameters to 'super'."));

				// This is a "super" expression. Handle it.
				return findTargetSuper(block, name, pos, params, thisVar);
			}

			LocalVarAccess *thisAccess = thisVar ? new (block) LocalVarAccess(pos, thisVar) : null;
			BSResolvePart *part = new (block) BSResolvePart(last->name, pos, params, thisAccess);
			if (!useThis)
				part->explicitFirst();
			name->last() = part;

			// Traverse the name tree.
			Named *found = scope.find(name);
			if (!found) {
				// Replace last with original part, so that re-tries work properly.
				name->last() = last;
				return new (block) UnresolvedName(block, name, pos, params, useThis);
			}

			// If it was a type with zero parameters, look for a constructor (explicit parameters
			// are handled earlier).
			if (Type *t = as<Type>(found))
				if (t->params->empty())
					return findCtorSafe(scope, t, params, pos);

			// If we used the this ptr, add it to params.
			Bool firstImplicit = false;
			if (found->params->count() == params->count() + 1) {
				params = params->withFirst(thisAccess);
				firstImplicit = true;
			}

			// Translate it into a suitable expression.
			return toExpression(scope, found, params, pos, firstImplicit, true);
		}

		static Expr *findTarget(Block *block, SimpleName *name, const SrcPos &pos, Actuals *params, bool useThis) {
			try {
				return findTargetI(block, name, pos, params, useThis);
			} catch (CodeError *error) {
				// Add position information if it is available.
				if (error->pos.empty())
					error->pos = pos;
				throw;
			}
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
