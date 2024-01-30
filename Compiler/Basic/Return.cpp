#include "stdafx.h"
#include "Return.h"
#include "Function.h"
#include "Cast.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		Return::Return(SrcPos pos, Block *block) : Expr(pos), retInfo(findReturn(pos, block)) {
			if (retInfo->type != Value()) {
				throw new (this) SyntaxError(pos, TO_S(this, S("Trying to return void from a function that returns ") << retInfo->type));
			}
		}

		Return::Return(SrcPos pos, Block *block, Expr *expr) : Expr(pos), retInfo(findReturn(pos, block)) {
			if (retInfo->type == Value())
				throw new (this) SyntaxError(pos, S("Trying to return a value from a function that returns void."));
			this->expr = expectCastTo(expr, retInfo->type, block->scope);
		}

		ExprResult Return::result() {
			return noReturn();
		}

		void Return::code(CodeGen *state, CodeResult *r) {
			if (retInfo->type == Value())
				voidCode(state);
			else
				valueCode(state);
		}

		void Return::voidCode(CodeGen *state) {
			using namespace code;

			if (retInfo->inlineResult) {
				*state->l << jmpBlock(retInfo->inlineLabel, retInfo->inlineBlock);
			} else {
				*state->l << fnRet();
			}
		}

		void Return::valueCode(CodeGen *state) {
			using namespace code;

			if (retInfo->inlineResult) {
				expr->code(state, retInfo->inlineResult);
				*state->l << jmpBlock(retInfo->inlineLabel, retInfo->inlineBlock);
			} else {
				// If we can get the result as a reference, do that as we avoid a copy. The exception
				// is built-in types, as we can copy them cheaply.
				Value type = retInfo->type;
				if (!type.isPrimitive() && expr->result().type().ref)
					type = retInfo->type.asRef();

				CodeResult *r = new (this) CodeResult(type, state->block);
				expr->code(state, r);

				if (type.ref)
					*state->l << fnRetRef(r->location(state));
				else
					state->returnValue(r->location(state));
			}
		}

		ReturnInfo *Return::findReturn(SrcPos pos, Block *block) {
			BlockLookup *lookup = block->lookup;

			while (lookup) {
				Block *block = lookup->block;
				if (ReturnPoint *root = as<ReturnPoint>(block))
					return root->info;
				if (FnBody *body = as<FnBody>(block))
					return body->info;

				lookup = as<BlockLookup>(lookup->parent());
			}

			throw new (this) SyntaxError(pos, S("A return statement is not located inside a function."));
		}

		void Return::toS(StrBuf *to) const {
			*to << S("return");
			if (expr)
				*to << S(" ") << expr;
		}

		ReturnInfo::ReturnInfo(Value type) : type(type) {}

		ReturnPoint::ReturnPoint(SrcPos pos, Scope scope, Value type) : Block(pos, scope) {
			info = new (this) ReturnInfo(type);
		}

		ReturnPoint::ReturnPoint(SrcPos pos, Block *parent, Value type) : Block(pos, parent) {
			info = new (this) ReturnInfo(type);
		}

		ExprResult ReturnPoint::result() {
			return info->type;
		}

		void ReturnPoint::body(Expr *e) {
			bodyExpr = expectCastTo(e, info->type, scope);
		}

		void ReturnPoint::blockCode(CodeGen *state, CodeResult *to, code::Block newBlock) {
			info->inlineResult = to;
			info->inlineLabel = state->l->label();
			info->inlineBlock = newBlock;

			*state->l << begin(newBlock);
			CodeGen *subState = state->child(newBlock);
			if (bodyExpr)
				bodyExpr->code(subState, to);
			*state->l << end(newBlock);
			*state->l << info->inlineLabel;
		}

	}
}
