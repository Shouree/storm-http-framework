#include "stdafx.h"
#include "Exception.h"
#include "Children.h"
#include "Node.h"
#include "Rule.h"
#include "Production.h"
#include "BSUtils.h"

#include "Compiler/Basic/Named.h"
#include "Compiler/Basic/WeakCast.h"
#include "Compiler/Basic/If.h"
#include "Compiler/Basic/For.h"

#include "Compiler/Lib/Array.h"
#include "Compiler/Lib/Maybe.h"

namespace storm {
	namespace syntax {
		using namespace bs;

		static bool interesting(Token *t) {
			if (!t->target)
				return false;

			Value type = t->target->type;
			if (!type.type)
				return false;

			type = unwrapArray(type);
			type = unwrapMaybe(type);

			return type.type->isA(Node::stormType(t->engine()));
		}

		static Value childResult(Engine &e) {
			return Value(Array<Node *>::stormType(e));
		}

		static syntax::SStr *childName(ProductionDecl *decl) {
			return new (decl) syntax::SStr(S("children"), decl->pos);
		}

		static Array<ValParam> *childParams(ProductionType *owner) {
			Array<ValParam> *p = new (owner) Array<ValParam>();
			p->push(thisParam(owner));
			return p;
		}

		ChildrenFn::ChildrenFn(ProductionDecl *decl, ProductionType *owner, Rule *rule, Scope scope)
			: BSRawFn(childResult(engine()), childName(decl), childParams(owner), null), scope(scope) {

			source = owner->production;
		}

		FnBody *ChildrenFn::createBody() {
			FnBody *root = new (this) FnBody(this, scope);

			// Create the variable holding the resulting array.
			LocalVarAccess *result = null;
			{
				Function *f = Array<Node *>::stormType(engine())->defaultCtor();
				Expr *r = new (this) CtorCall(pos, scope, f, new (this) Actuals());
				Var *v = new (this) Var(root, new (this) syntax::SStr(S("result")), r);
				root->add(v);
				result = new (this) LocalVarAccess(pos, v->var);
			}

			// Go through the rule and generate code!
			addTokens(root, result);

			// Return 'result'.
			root->add(result);
			return root;
		}

		void ChildrenFn::addTokens(ExprBlock *block, Expr *result) {
			Nat tokens = tokenCount();
			Nat firstBreak = min(source->repStart, tokens);
			Nat secondBreak = min(source->repEnd, tokens);

			// Tokens before the capture.
			for (Nat i = 0; i < firstBreak; i++)
				addToken(block, result, getToken(i));

			// Tokens in the capture.
			switch (source->repType) {
			case repZeroOne:
				// Only Maybe<T>. If statement is sufficient!
				for (Nat i = firstBreak; i < secondBreak; i++)
					addTokenIf(block, result, getToken(i));
				break;
			case repOnePlus:
			case repZeroPlus:
				// Only Array<T>. Embed inside for-loop.
				addTokenLoop(block, result, firstBreak, secondBreak);
				break;
			default:
				// Nothing special!
				for (Nat i = firstBreak; i < secondBreak; i++)
					addToken(block, result, getToken(i));
				break;
			}

			// Tokens after the capture.
			for (Nat i = secondBreak; i < tokens; i++)
				addToken(block, result, getToken(i));
		}

		void ChildrenFn::addToken(ExprBlock *block, Expr *result, Token *token) {
			// Not stored?
			if (!interesting(token))
				return;

			Expr *src = new (this) MemberVarAccess(pos, thisExpr(block), token->target, true);
			block->add(push(block, result, src));
		}

		void ChildrenFn::addTokenIf(ExprBlock *block, Expr *result, Token *token) {
			if (!interesting(token))
				return;

			Expr *src = new (this) MemberVarAccess(pos, thisExpr(block), token->target, true);
			WeakCast *cast = new (this) WeakMaybeCast(src);
			If *check = new (this) If(block, cast);

			LocalVar *overwrite = check->condition->result();
			assert(overwrite, L"Weak cast did not overwrite variable as expected.");
			check->success(push(check->successBlock, result, new (this) LocalVarAccess(pos, overwrite)));
			block->add(check);
		}

		void ChildrenFn::addTokenLoop(bs::ExprBlock *block, bs::Expr *result, Nat from, Nat to) {
			Engine &e = engine();

			// Find the minimal length to iterate...
			Expr *minExpr = null;
			for (Nat i = from; i < to; i++) {
				Token *t = getToken(i);
				if (!interesting(t))
					// Not interesting...
					continue;

				Expr *src = new (this) MemberVarAccess(pos, thisExpr(block), t->target, true);
				Expr *read = callMember(scope, S("count"), src);

				if (minExpr)
					minExpr = callMember(scope, S("min"), minExpr, read);
				else
					minExpr = read;
			}

			if (!minExpr)
				// Nothing interesting. Abort!
				return;

			ExprBlock *forBlock = new (this) ExprBlock(pos, block);
			Var *end = new (this) Var(forBlock,
									Value(StormInfo<Nat>::type(e)),
									new (e) syntax::SStr(S("_end")),
									minExpr);
			forBlock->add(end);
			Expr *readEnd = new (this) LocalVarAccess(pos, end->var);

			// Iterate...
			Var *i = new (this) Var(forBlock,
									Value(StormInfo<Nat>::type(e)),
									new (e) syntax::SStr(S("_i")),
									new (this) NumLiteral(pos, 0));
			Expr *readI = new (this) LocalVarAccess(pos, i->var);
			forBlock->add(i);

			For *loop = new (this) For(pos, forBlock);
			loop->test(callMember(scope, S("<"), readI, readEnd));
			loop->update(callMember(scope, S("++*"), readI));

			ExprBlock *inLoop = new (this) ExprBlock(pos, loop);
			for (Nat i = from; i < to; i++) {
				Token *t = getToken(i);
				if (!interesting(t))
					continue;

				Expr *src = new (this) MemberVarAccess(pos, thisExpr(block), t->target, true);
				Expr *read = callMember(scope, S("[]"), src, readI);
				inLoop->add(push(inLoop, result, read));
			}

			loop->body(inLoop);
			forBlock->add(loop);
			block->add(forBlock);
		}

		Expr *ChildrenFn::push(Block *block, Expr *result, Expr *tokenSrc) {
			return callMember(pos, scope, S("push"), result, tokenSrc);
		}

		bs::Expr *ChildrenFn::thisExpr(bs::ExprBlock *block) {
			LocalVar *var = block->variable(new (this) SimplePart(new (this) Str(S("this"))));
			assert(var, L"'this' was not found!");
			return new (this) LocalVarAccess(pos, var);
		}

		Nat ChildrenFn::tokenCount() {
			Nat r = source->tokens->count();
			if (source->repCapture)
				r++;
			return r;
		}

		Token *ChildrenFn::getToken(Nat pos) {
			Array<Token *> *tokens = source->tokens;
			if (pos == tokens->count())
				return source->repCapture;
			else
				return tokens->at(pos);
		}


		Function *createChildrenFn(ProductionDecl *decl, ProductionType *owner, Scope scope) {
			return new (decl) ChildrenFn(decl, owner, owner->rule(), scope);
		}

	}
}
