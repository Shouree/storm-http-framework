#include "stdafx.h"
#include "Return.h"
#include "Function.h"
#include "Cast.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Return::Return(SrcPos pos, Block *block) : Expr(pos), body(findParentFn(pos, block)) {
			if (body->type != Value()) {
				throw new (this) SyntaxError(pos, TO_S(this, S("Trying to return void from a function that returns ") << body->type));
			}
		}

		Return::Return(SrcPos pos, Block *block, Expr *expr) : Expr(pos), body(findParentFn(pos, block)) {
			if (body->type == Value())
				throw new (this) SyntaxError(pos, S("Trying to return a value from a function that returns void."));
			this->expr = expectCastTo(expr, body->type, block->scope);
		}

		ExprResult Return::result() {
			return noReturn();
		}

		void Return::code(CodeGen *state, CodeResult *r) {
			if (body->type == Value())
				voidCode(state);
			else
				valueCode(state);
		}

		void Return::voidCode(CodeGen *state) {
			using namespace code;

			if (body->inlineResult) {
				*state->l << jmpBlock(body->inlineLabel, body->inlineBlock);
			} else {
				*state->l << fnRet();
			}
		}

		void Return::valueCode(CodeGen *state) {
			using namespace code;

			if (body->inlineResult) {
				expr->code(state, body->inlineResult);
				*state->l << jmpBlock(body->inlineLabel, body->inlineBlock);
			} else {
				// If we can get the result as a reference, do that as we avoid a copy. The exception
				// is built-in types, as we can copy them cheaply.
				Value type = body->type;
				if (!type.isPrimitive() && expr->result().type().ref)
					type = body->type.asRef();

				CodeResult *r = new (this) CodeResult(type, state->block);
				expr->code(state, r);

				if (type.ref)
					*state->l << fnRetRef(r->location(state));
				else
					state->returnValue(r->location(state));
			}
		}

		FnBody *Return::findParentFn(SrcPos pos, Block *block) {
			BlockLookup *lookup = block->lookup;

			while (lookup) {
				Block *block = lookup->block;
				if (FnBody *root = as<FnBody>(block))
					return root;

				lookup = as<BlockLookup>(lookup->parent());
			}

			throw new (this) SyntaxError(pos, S("A return statement is not located inside a function."));
		}

		void Return::toS(StrBuf *to) const {
			*to << S("return");
			if (expr)
				*to << S(" ") << expr;
		}

	}
}
