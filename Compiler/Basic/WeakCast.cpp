#include "stdafx.h"
#include "WeakCast.h"
#include "Named.h"
#include "Core/Variant.h"
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

		MAYBE(Str *) WeakCast::overwrite() {
			return null;
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

			if (MemberVarAccess *var = as<MemberVarAccess>(expr))
				if (var->implicitMember())
					return new (this) Str(*var->var->name);

			return null;
		}

		void WeakCast::trueCode(CodeGen *state) {
			if (LocalVar *var = result())
				initCode(state, var);
		}


		/**
		 * 'as' cast.
		 */

		WeakAsCast::WeakAsCast(Block *block, Expr *expr, SrcName *type)
			: expr(expr), to(block->scope.value(type)) {

			if (to == Value())
				throw new (this) SyntaxError(expr->pos, S("Can not cast to void."));
		}

		SrcPos WeakAsCast::pos() {
			return expr->largePos();
		}

		MAYBE(Str *) WeakAsCast::overwrite() {
			return defaultOverwrite(expr);
		}

		Value WeakAsCast::resultType() {
			return to;
		}

		void WeakAsCast::toS(StrBuf *to) const {
			output(to);
			*to << expr << S(" as ") << this->to;
		}


		/**
		 * Downcast.
		 */

		WeakDowncast::WeakDowncast(Block *block, Expr *expr, SrcName *type) : WeakAsCast(block, expr, type) {
			Value from = expr->result().type();
			if (isMaybe(from))
				from = unwrapMaybe(from);

			if (!from.isObject())
				throw new (this) SyntaxError(expr->pos, S("The WeakDowncast class can only be used with object types."));
			if (!to.type->isA(from.type)) {
				Str *msg = TO_S(engine(), S("Condition is always false. ") << to
								<< S(" does not inherit from ") << from << S("."));
				throw new (this) SyntaxError(expr->pos, msg);
			}
		}

		void WeakDowncast::code(CodeGen *state, CodeResult *boolResult) {
			using namespace code;

			// Get the value...
			Value fromType = expr->result().type();
			CodeResult *from = new (this) CodeResult(fromType, state->block);
			expr->code(state, from);

			// Remember where we stored the value...
			exprVar = from->location(state);
			exprVarRef = fromType.ref;

			// Load into eax...
			if (fromType.ref) {
				*state->l << mov(ptrA, exprVar);
				*state->l << mov(ptrA, ptrRel(ptrA, Offset()));
			} else {
				*state->l << mov(ptrA, exprVar);
			}

			// Call the 'as' function.
			*state->l << fnParam(engine().ptrDesc(), ptrA);
			*state->l << fnParam(engine().ptrDesc(), to.type->typeRef());
			*state->l << fnCall(engine().ref(builtin::as), false, engine().ptrDesc(), ptrA);
			*state->l << cmp(ptrA, ptrConst(Offset()));
			*state->l << setCond(al, ifNotEqual);
			*state->l << mov(boolResult->location(state), al);
		}

		void WeakDowncast::initCode(CodeGen *state, LocalVar *var) {
			using namespace code;

			// Now we are ready to update the variable!
			if (exprVarRef) {
				*state->l << mov(ptrA, exprVar);
				*state->l << mov(var->var.v, ptrRel(ptrA, Offset()));
			} else {
				*state->l << mov(var->var.v, exprVar);
			}
			var->var.created(state);
		}


		/**
		 * VariantCast.
		 */

		WeakVariantCast::WeakVariantCast(Block *block, Expr *expr, SrcName *type) : WeakAsCast(block, expr, type) {
			variantType = StormInfo<Variant>::type(engine());
			if (expr->result().type().type != variantType)
				throw new (this) SyntaxError(expr->pos, S("Can not use WeakVariantCast with anything except variants."));
		}

		void WeakVariantCast::code(CodeGen *state, CodeResult *boolResult) {
			using namespace code;

			// Get the source value. Try to get a reference to avoid copying.
			Value fromType = expr->result().type();
			CodeResult *from = new (this) CodeResult(fromType, state->block);
			expr->code(state, from);

			exprVar = from->location(state);
			exprVarRef = fromType.ref;

			// Call the 'as' function to see if the value is of the correct type.
			Array<Value> *params = new (this) Array<Value>();
			params->push(thisPtr(variantType));
			params->push(StormInfo<Type>::type(engine()));
			Function *hasFn = as<Function>(variantType->find(S("has"), params, Scope()));
			if (!hasFn)
				throw new (this) InternalError(S("Can not find Variant::has()."));

			Array<Operand> *operands = new (this) Array<Operand>();
			if (exprVarRef) {
				operands->push(exprVar);
			} else {
				*state->l << lea(ptrA, exprVar);
				operands->push(ptrA);
			}
			operands->push(to.type->typeRef());
			hasFn->localCall(state, operands, boolResult, false);
		}

		void WeakVariantCast::initCode(CodeGen *state, LocalVar *var) {
			using namespace code;

			// Now we can copy the data from the Variant into our variable!
			if (to.isObject()) {
				// Objects are quite easy to deal with: the Variant simply contains a pointer to the object.
				if (exprVarRef) {
					*state->l << mov(ptrA, exprVar);
					*state->l << mov(var->var.v, ptrRel(ptrA, Offset()));
				} else {
					*state->l << mov(var->var.v, exprVar);
				}
			} else {
				// Values are a bit trickier. The Variant contains a pointer to an array of size 1
				// that contains the data. Load the array ptr into ptrA.
				if (exprVarRef) {
					*state->l << mov(ptrA, exprVar);
					*state->l << mov(ptrA, ptrRel(ptrA, Offset()));
				} else {
					*state->l << mov(ptrA, exprVar);
				}

				// Adjust ptrA so that it skips the array header:
				*state->l << add(ptrA, ptrConst(Size::sPtr * 2));

				// Now, we can copy the data!
				if (to.isAsmType()) {
					*state->l << mov(var->var.v, xRel(to.size(), ptrA, Offset()));
				} else {
					*state->l << lea(ptrC, var->var.v);
					*state->l << fnParam(engine().ptrDesc(), ptrC);
					*state->l << fnParam(engine().ptrDesc(), ptrA);
					*state->l << fnCall(to.copyCtor(), true);
				}
			}
			var->var.created(state);
		}


		/**
		 * MaybeCast.
		 */

		WeakMaybeCast::WeakMaybeCast(Expr *expr) : WeakCast(), expr(expr) {}

		SrcPos WeakMaybeCast::pos() {
			return expr->largePos();
		}

		MAYBE(Str *) WeakMaybeCast::overwrite() {
			return defaultOverwrite(expr);
		}

		Value WeakMaybeCast::resultType() {
			return unwrapMaybe(expr->result().type());
		}

		void WeakMaybeCast::code(CodeGen *state, CodeResult *boolResult) {
			exprType = expr->result().type();
			if (MaybeClassType *c = as<MaybeClassType>(exprType.type)) {
				classCode(c, state, boolResult);
			} else if (MaybeValueType *v = as<MaybeValueType>(exprType.type)) {
				valueCode(v, state, boolResult);
			}
		}

		void WeakMaybeCast::initCode(CodeGen *state, LocalVar *var) {
			if (MaybeClassType *c = as<MaybeClassType>(exprType.type)) {
				classInit(c, state, var);
			} else if (MaybeValueType *v = as<MaybeValueType>(exprType.type)) {
				valueInit(v, state, var);
			}
		}

		void WeakMaybeCast::classCode(MaybeClassType *c, CodeGen *state, CodeResult *boolResult) {
			using namespace code;

			// Store directly in the result variable if possible. We create it earlier so that this
			// is possible. It is not a problem since it is a pointer variable anyway.
			LocalVar *var = result();

			CodeResult *from;
			if (var) {
				var->create(state);
				from = new (this) CodeResult(Value(c), var->var);
			} else {
				from = new (this) CodeResult(Value(c), state->block);
			}
			expr->code(state, from);

			// Check if it is null.
			*state->l << cmp(from->location(state), ptrConst(Offset()));
			*state->l << setCond(boolResult->location(state), ifNotEqual);
		}

		void WeakMaybeCast::classInit(MaybeClassType *c, CodeGen *state, LocalVar *var) {
			// Nothing to do: we do everything in 'classCode' to save local variables.
			(void)c;
			(void)state;
			(void)var;
		}

		void WeakMaybeCast::valueCode(MaybeValueType *c, CodeGen *state, CodeResult *boolResult) {
			using namespace code;

			// We're asking for a reference if possible so we don't have to copy.
			CodeResult *from = new (this) CodeResult(exprType, state->block);
			expr->code(state, from);

			// Check if it is null.
			exprVar = from->location(state);
			if (exprType.ref)
				*state->l << mov(ptrC, exprVar);
			else
				*state->l << lea(ptrC, exprVar);
			*state->l << cmp(byteRel(ptrC, c->boolOffset()), byteConst(0));
			*state->l << setCond(boolResult->location(state), ifNotEqual);
		}

		void WeakMaybeCast::valueInit(MaybeValueType *c, CodeGen *state, LocalVar *var) {
			using namespace code;

			if (exprType.ref)
				*state->l << mov(ptrC, exprVar);
			else
				*state->l << lea(ptrC, exprVar);

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

		void WeakMaybeCast::toS(StrBuf *to) const {
			output(to);
			*to << expr;
		}


		/**
		 * Helpers.
		 */

		WeakCast *weakAsCast(Block *block, Expr *expr, SrcName *type) {
			Value exprType = expr->result().type();

			// Variant cast:
			if (exprType.type == StormInfo<Variant>::type(expr->engine())) {
				return new (expr) WeakVariantCast(block, expr, type);
			}

			// Downcast:
			{
				Value noMaybe = unwrapMaybe(exprType);
				if (noMaybe.isObject()) {
					return new (expr) WeakDowncast(block, expr, type);
				}
			}

			// Nothing useful to give.
			const wchar *msg = S("The 'as' operator can only be used for class types, actor types, and Variants.");
			throw new (expr) SyntaxError(expr->pos, msg);
		}


	}
}
