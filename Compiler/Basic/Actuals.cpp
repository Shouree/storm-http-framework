#include "stdafx.h"
#include "Actuals.h"
#include "Cast.h"
#include "Compiler/CodeGen.h"
#include "Core/Join.h"

namespace storm {
	namespace bs {

		Actuals::Actuals() {
			expressions = new (this) Array<Expr *>();
		}

		Actuals::Actuals(Expr *expr) {
			expressions = new (this) Array<Expr *>();
			add(expr);
		}

		Actuals::Actuals(Array<Expr *> *exprs) {
			expressions = new (this) Array<Expr *>(*exprs);
		}

		Actuals::Actuals(const Actuals &o) : ObjectOn<Compiler>(o) {
			expressions = new (this) Array<Expr *>(*o.expressions);
		}

		static Array<Value> *values(Array<Expr *> *expressions) {
			Array<Value> *v = new (expressions) Array<Value>();
			v->reserve(expressions->count());
			for (nat i = 0; i < expressions->count(); i++)
				v->push(expressions->at(i)->result().type());

			return v;
		}

		Array<Value> *Actuals::values() {
			return storm::bs::values(expressions);
		}

		void Actuals::add(Expr *e) {
			expressions->push(e);
		}

		void Actuals::addFirst(Expr *e) {
			expressions->insert(0, e);
		}

		Actuals *Actuals::withFirst(Expr *e) const {
			Actuals *copy = new (this) Actuals(*this);
			copy->addFirst(e);
			return copy;
		}

		Bool Actuals::hasThisFirst() const {
			if (expressions->any())
				return expressions->at(0)->thisVariable();
			return false;
		}

		/**
		 * Helper to compute an actual parameter. Takes care of ref/non-ref conversions.
		 * Returns the value into which the resulting parameter were placed.
		 */
		code::Operand Actuals::code(nat id, CodeGen *s, Value param, Scope scope) {
			using namespace code;

			Expr *expr = castTo(expressions->at(id), param, scope);
			if (!expr && param.ref) {
				// If we failed, try to cast to a non-reference type and deal with that later.
				expr = castTo(expressions->at(id), param.asRef(false), scope);
			}
			assert(expr,
				L"Can not use " + ::toS(expressions->at(id)->result()) + L" as an actual value for parameter " + ::toS(param));

			// Type to ask the expression for. This might differ from the type returned, since the
			// expression might choose to do late-stage typecasting through the 'castPenalty'
			// interface.
			Value exprResult = expr->result().type();
			Value askFor = param;

			// If 'exprResult' returns a value type that is a derived type of what we are asking
			// for, ask for the derived type instead. Otherwise, we might not allocate enough memory
			// for the derived value. We don't need to bother if we are dealing with references all
			// the way.
			if (!param.ref || !exprResult.ref) {
				if (askFor.type != exprResult.type && askFor.isValue() && askFor.mayReferTo(exprResult.asRef(false))) {
					askFor = exprResult.asRef(false);
				}
			}

			// Do we need to create a reference?
			if (param.ref && !exprResult.ref) {
				// We need to create a temporary variable and make a reference to it.
				askFor.ref = false;
				CodeResult *gr = new (this) CodeResult(askFor, s->block);
				expr->code(s, gr);

				VarInfo tmpRef = s->createVar(param);
				*s->l << lea(tmpRef.v, ptrRel(gr->location(s), Offset()));
				tmpRef.created(s);
				return tmpRef.v;
			} else {
				// 'expr' will handle the type we are giving it. If it reported that it can provide
				// a reference, it needs to be able to provide a value as well.
				CodeResult *gr = new (this) CodeResult(askFor, s->block);
				expr->code(s, gr);

				// If it was not a reference, we might need to perform "slicing" to not confuse the
				// remainder of the system.
				if (!askFor.ref)
					return xRel(param.size(), gr->location(s), Offset());
				else
					return gr->location(s);
			}
		}

		void Actuals::toS(StrBuf *to) const {
			*to << S("(") << join(expressions, S(", ")) << S(")");
		}


		BSNamePart::BSNamePart(syntax::SStr *name, Actuals *params) :
			SimplePart(name->v, params->values()), pos(name->pos) {

			exprs = new (this) Array<Expr *>(*params->expressions);
		}

		BSNamePart::BSNamePart(Str *name, SrcPos pos, Actuals *params) :
			SimplePart(name, params->values()), pos(pos) {

			exprs = new (this) Array<Expr *>(*params->expressions);
		}

		BSNamePart::BSNamePart(const wchar *name, SrcPos pos, Actuals *params) :
			SimplePart(new (this) Str(name), params->values()), pos(pos) {

			exprs = new (this) Array<Expr *>(*params->expressions);
		}

		BSNamePart::BSNamePart(Str *name, SrcPos pos, Array<Expr *> *params) :
			SimplePart(name, values(params)), pos(pos), exprs(params) {}

		void BSNamePart::insert(Expr *first) {
			insert(first, 0);
		}

		void BSNamePart::insert(Expr *first, Nat at) {
			params->insert(at, first->result().type());
			exprs->insert(at, first);
		}

		void BSNamePart::insert(Value first) {
			insert(first, 0);
		}

		void BSNamePart::insert(Value first, Nat at) {
			params->insert(at, first);
			exprs->insert(at, new (this) DummyExpr(pos, first));
		}

		void BSNamePart::alter(Nat at, Value to) {
			params->at(at) = to;
			exprs->at(at) = new (this) DummyExpr(pos, to);
		}

		void BSNamePart::strictFirst() {
			strictThis = true;
		}

		void BSNamePart::explicitFirst() {
			expFirst = true;
		}

		Bool BSNamePart::scopeParam(Nat id) const {
			return (id == 0) & expFirst;
		}

		BSNamePart *BSNamePart::withFirst(Value val) const {
			Array<Expr *> *exprs = new (this) Array<Expr *>();
			exprs->reserve(this->exprs->count() + 1);
			*exprs << new (this) DummyExpr(pos, val);
			for (Nat i = 0; i < this->exprs->count(); i++)
				*exprs << this->exprs->at(i);
			return new (this) BSNamePart(name, pos, exprs);
		}

		// TODO: Consider using 'max' for match weights instead?
		// TODO: Consider storing a context inside the context, so that names are resolved in the context
		// they were created rather than in the context they are evaluated. Not sure if this is a good idea though.
		Int BSNamePart::matches(Named *candidate, Scope context) const {
			Array<Value> *c = candidate->params;
			if (c->count() != params->count())
				return -1;

			Int distance = 0;

			for (Nat i = 0; i < c->count(); i++) {
				Int penalty = checkParam(c->at(i), i, i == 0, candidate->flags, context);

				if (penalty >= 0)
					distance += penalty;
				else
					return -1;
			}

			return distance;
		}

		Int BSNamePart::checkParam(Value formal, Nat index, Bool first, NamedFlags flags, Scope context) const {
			return checkParam(formal, exprs->at(index), first, flags, context);
		}

		Int BSNamePart::checkParam(Value formal, Expr *actual, Bool first, NamedFlags flags, Scope context) const {
			// We can convert everything to references, so treat everything as if it was a plain
			// value. However, we don't want to use automatic type casts when the formal
			// parameter is a reference, since that eliminates any side effects (most notably
			// for the 'this' parameter).
			if (formal.ref && first && strictThis) {
				return plainCastPenalty(actual->result(), formal.asRef(false), flags);
			} else {
				return castPenalty(actual, formal.asRef(false), flags, context);
			}
		}

		Name *bsName(syntax::SStr *name, Actuals *params) {
			return new (params) Name(new (params) BSNamePart(name, params));
		}

		Name *bsName(Str *name, SrcPos pos, Actuals *params) {
			return new (params) Name(new (params) BSNamePart(name, pos, params));
		}

	}
}
