#include "stdafx.h"
#include "WeakCast.h"
#include "Named.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace bs {

		WeakCast::WeakCast() {}

		MAYBE(LocalVar *) STORM_FN WeakCast::result() {
			if (created)
				return created;

			Str *name = null;
			SrcPos pos;
			if (varName) {
				name = varName->v;
				pos = varName->pos;
			} else {
				name = overwrite();
				pos = this->pos();
			}

			if (!name)
				return null;

			created = new (this) LocalVar(name, resultType(), pos, false);
			return created;
		}

		void WeakCast::code(CodeGen *state, CodeResult *ok) {
			LocalVar *var = result();
			if (var)
				var->create(state);

			castCode(state, ok, var);
		}

		MAYBE(Str *) WeakCast::overwrite() {
			return null;
		}

		void WeakCast::castCode(CodeGen *state, CodeResult *ok, MAYBE(LocalVar *) var) {
			using namespace code;

			// Always indicate cast faile.d
			*state->l << mov(ok->location(state), byteConst(0));
		}

		void WeakCast::name(syntax::SStr *name) {
			varName = name;
		}

		void WeakCast::output(StrBuf *to) const {
			if (varName) {
				*to << varName->v;
				*to << S(" = ");
			}
		}

		MAYBE(Str *) WeakCast::defaultOverwrite(Expr *expr) {
			if (LocalVarAccess *var = as<LocalVarAccess>(expr))
				return new (this) Str(*var->var->name);

			// TODO? Enforce 'this' is used as the parameter?
			if (MemberVarAccess *var = as<MemberVarAccess>(expr))
				return new (this) Str(*var->var->name);

			return null;
		}


		/**
		 * 'as' cast.
		 */

		WeakDowncast::WeakDowncast(Block *block, Expr *expr, SrcName *type)
			: WeakCast(), expr(expr) {

			to = block->scope.value(type);

			Value from = expr->result().type();
			if (isMaybe(from))
				from = unwrapMaybe(from);

			if (!from.isObject())
				throw new (this) SyntaxError(expr->pos, S("The 'as' operator is only applicable to class or actor types."));
			if (!to.type->isA(from.type)) {
				Str *msg = TO_S(engine(), S("Condition is always false. ") << to
								<< S(" does not inherit from ") << from << S("."));
				throw new (this) SyntaxError(expr->pos, msg);
			}
		}

		SrcPos WeakDowncast::pos() {
			return expr->pos;
		}

		MAYBE(Str *) WeakDowncast::overwrite() {
			return defaultOverwrite(expr);
		}

		Value WeakDowncast::resultType() {
			return to;
		}

		void WeakDowncast::castCode(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			// Get the value...
			Value fromType = expr->result().type();
			CodeResult *from = new (this) CodeResult(fromType, state->block);
			expr->code(state, from);

			// Load into eax...
			if (fromType.ref) {
				*state->l << mov(ptrA, from->location(state));
				*state->l << mov(ptrA, ptrRel(ptrA, Offset()));
			} else {
				*state->l << mov(ptrA, from->location(state));
			}

			// Store the object into 'var' regardless if the test succeeded. It does not matter.
			if (var) {
				*state->l << mov(var->var.v, ptrA);
				var->var.created(state);
			}

			// Call the 'as' function.
			*state->l << fnParam(engine().ptrDesc(), ptrA);
			*state->l << fnParam(engine().ptrDesc(), to.type->typeRef());
			*state->l << fnCall(engine().ref(builtin::as), false, engine().ptrDesc(), ptrA);
			*state->l << cmp(ptrA, ptrConst(Offset()));
			*state->l << setCond(al, ifNotEqual);
			*state->l << mov(boolResult->location(state), al);
		}

		void WeakDowncast::toS(StrBuf *to) const {
			output(to);
			*to << expr << S(" as ") << this->to;
		}

		/**
		 * MaybeCast.
		 */

		WeakMaybeCast::WeakMaybeCast(Expr *expr) : WeakCast(), expr(expr) {}

		SrcPos WeakMaybeCast::pos() {
			return expr->pos;
		}

		MAYBE(Str *) WeakMaybeCast::overwrite() {
			return defaultOverwrite(expr);
		}

		Value WeakMaybeCast::resultType() {
			return unwrapMaybe(expr->result().type());
		}

		void WeakMaybeCast::castCode(CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			Value srcType = expr->result().type();
			if (MaybeClassType *c = as<MaybeClassType>(srcType.type)) {
				classCode(c, state, boolResult, var);
			} else if (MaybeValueType *v = as<MaybeValueType>(srcType.type)) {
				valueCode(v, srcType.ref, state, boolResult, var);
			}
		}

		void WeakMaybeCast::classCode(MaybeClassType *c, CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			CodeResult *from;
			if (var)
				from = new (this) CodeResult(Value(c), var->var);
			else
				from = new (this) CodeResult(Value(c), state->block);
			expr->code(state, from);

			// Check if it is null.
			*state->l << cmp(from->location(state), ptrConst(Offset()));
			*state->l << setCond(boolResult->location(state), ifNotEqual);
		}

		void WeakMaybeCast::valueCode(MaybeValueType *c, Bool ref, CodeGen *state, CodeResult *boolResult, MAYBE(LocalVar *) var) {
			using namespace code;

			// We're asking for a reference if possible so we don't have to copy.
			CodeResult *from = new (this) CodeResult(Value(c, ref), state->block);
			expr->code(state, from);

			Label end = state->l->label();

			// Check if it is null.
			if (ref)
				*state->l << mov(ptrC, from->location(state));
			else
				*state->l << lea(ptrC, from->location(state));
			*state->l << cmp(byteRel(ptrC, c->boolOffset()), byteConst(0));
			*state->l << setCond(boolResult->location(state), ifNotEqual);
			*state->l << jmp(end, ifEqual);

			// Copy it if we want a result, and if we weren't null.
			if (var) {
				*state->l << lea(ptrA, var->var.v);
				if (c->param().isAsmType()) {
					Size sz = c->param().size();
					*state->l << mov(xRel(sz, ptrA, Offset()), xRel(sz, ptrC, Offset()));
				} else {
					*state->l << fnParam(engine().ptrDesc(), ptrA);
					*state->l << fnParam(engine().ptrDesc(), ptrC);
					*state->l << fnCall(c->param().copyCtor(), true);
				}
				var->var.created(state);
			}

			*state->l << end;
		}

		void WeakMaybeCast::toS(StrBuf *to) const {
			output(to);
			*to << expr;
		}


		/**
		 * Helpers.
		 */

		WeakCast *weakAsCast(Block *block, Expr *expr, SrcName *type) {
			// Currently, only downcast is supported.
			return new (expr) WeakDowncast(block, expr, type);
		}


	}
}
