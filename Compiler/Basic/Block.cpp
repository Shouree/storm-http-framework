#include "stdafx.h"
#include "Block.h"
#include "Exception.h"
#include "NamePart.h"
#include "Core/StrBuf.h"

namespace storm {
	namespace bs {

		Block::Block(SrcPos pos, Scope scope) : Expr(pos) {
			lookup = new (this) BlockLookup(this, scope.top);
			this->scope = Scope(scope, lookup);
			variables = new (this) VarMap();
		}

		Block::Block(SrcPos pos, Block *parent) : Expr(pos) {
			lookup = new (this) BlockLookup(this, parent->scope.top);
			scope = Scope(parent->scope, lookup);
			variables = new (this) VarMap();
		}

		void Block::initVariables(CodeGen *child) {
			for (VarMap::Iter i = variables->begin(), end = variables->end(); i != end; ++i) {
				i.v()->create(child);
			}
		}

		void Block::code(CodeGen *state, CodeResult *to) {
			using namespace code;

			code::Block block = state->l->createBlock(state->block);
			CodeGen *child = state->child(block);

			initVariables(child);

			blockCode(state, to, block);
		}

		void Block::blockCode(CodeGen *state, CodeResult *to, code::Block block) {
			*state->l << begin(block);
			CodeGen *subState = state->child(block);
			blockCode(subState, to);
			*state->l << end(block);
		}

		void Block::blockCode(CodeGen *state, CodeResult *to) {
			throw new (this) AbstractFnCalled(S("storm::Block::blockCode"));
		}

		void Block::add(LocalVar *var) {
			if (variables->has(var->name)) {
				// Old position: variables->get(var->name)->pos
				Str *msg = TO_S(engine(), S("The variable ") << var->name
								<< S(" is already defined in this block:\n@")
								<< variables->get(var->name)->pos
								<< S(": Syntax error: Previously defined here."));
				throw new (this) TypeError(var->pos, msg);
			}
			variables->put(var->name, var);
		}

		void Block::liftVars(Block *from) {
			assert(isParentTo(from), L"'liftVars' can only be used to move variables from child to parent, one step.");

			for (VarMap::Iter i = from->variables->begin(); i != from->variables->end(); ++i) {
				add(i.v());
			}

			from->variables->clear();
		}

		MAYBE(Block *) Block::parent() {
			return as<Block>(lookup->parentLookup);
		}

		bool Block::isParentTo(Block *x) {
			return x->lookup->parent() == lookup;
		}

		LocalVar *Block::variable(SimplePart *part) {
			// Don't try to search for parameterized variables, that makes no sense at the moment!
			if (part->params->any())
				return null;

			if (LocalVar *found = variableHere(part))
				return found;

			BlockLookup *parent = as<BlockLookup>(lookup->parent());
			if (parent)
				return parent->block->variable(part);

			return null;
		}

		LocalVar *Block::variableHere(SimplePart *part) {
			if (part->params->any())
				return null;

			Str *name = part->name;
			VarMap::Iter i = variables->find(name);
			if (i != variables->end())
				return i.v();

			return null;
		}

		Bool Block::isolate() {
			return false;
		}


		ExprBlock::ExprBlock(SrcPos pos, Scope scope) :
			Block(pos, scope), exprs(new (this) Array<Expr *>()) {}

		ExprBlock::ExprBlock(SrcPos pos, Block *parent) :
			Block(pos, parent), exprs(new (this) Array<Expr *>()) {}

		void ExprBlock::add(Expr *expr) {
			exprs->push(expr);
		}

		void ExprBlock::insert(Nat pos, Expr *expr) {
			exprs->insert(pos, expr);
		}

		ExprResult ExprBlock::result() {
			if (exprs->empty())
				return ExprResult();

			for (Nat i = 0; i < exprs->count() - 1; i++) {
				if (exprs->at(i)->result().nothing())
					return noReturn();
			}

			return exprs->last()->result();
		}

		void ExprBlock::code(CodeGen *state, CodeResult *to) {
			if (exprs->any())
				Block::code(state, to);
		}

		// Execute 'expr' in a sub-block as necessary.
		static void executeExpr(Expr *expr, CodeGen *state, CodeResult *to) {
			if (expr->isolate()) {
				CodeGen *child = state->child();
				*state->l << begin(child->block);
				expr->code(child, to);
				*state->l << end(child->block);
			} else {
				expr->code(state, to);
			}
		}

		static void putLocation(CodeGen *state, SrcPos &last, SrcPos current) {
			if (last != current) {
				last = current;
				*state->l << code::location(current);
			}
		}

		void ExprBlock::blockCode(CodeGen *state, CodeResult *to) {
			if (!exprs->any())
				return;

			SrcPos last = pos;

			// Generate code for the entire block. Stop whenever we find a block that does not return.
			for (Nat i = 0; i < exprs->count() - 1; i++) {
				Expr *e = exprs->at(i);
				putLocation(state, last, e->largePos());

				CodeResult *s = new (this) CodeResult();
				executeExpr(e, state, s);

				// Stop if this statement never returns.
				if (e->result().nothing())
					return;
			}

			putLocation(state, last, exprs->last()->largePos());
			executeExpr(exprs->last(), state, to);

			putLocation(state, last, pos.lastCh());
		}

		Int ExprBlock::castPenalty(Value to) {
			if (exprs->empty())
				return -1;
			return exprs->last()->castPenalty(to);
		}

		void ExprBlock::toS(StrBuf *to) const {
			*to << S("{\n");
			{
				Bool unreachable = false;
				Indent i(to);
				for (nat i = 0; i < exprs->count(); i++) {
					*to << exprs->at(i) << S(";\n");
					if (i != exprs->count() - 1 && exprs->at(i)->result().nothing()) {
						if (!unreachable)
							*to << S("// unreachable code:\n");
						unreachable = true;
					}
				}
			}
			*to << S("}");
		}

		Expr *ExprBlock::operator [](Nat at) const {
			return exprs->at(at);
		}

		Nat ExprBlock::count() const {
			return exprs->count();
		}

		/**
		 * Block lookup
		 */

		BlockLookup::BlockLookup(Block *o, NameLookup *prev) : NameLookup(prev), block(o) {}

		Named *BlockLookup::find(SimplePart *part, Scope source) {
			if (part->params->empty())
				return block->variableHere(part);

			return null;
		}

	}
}
