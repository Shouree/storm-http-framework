#include "stdafx.h"
#include "Var.h"
#include "Block.h"
#include "Cast.h"
#include "Named.h"
#include "Type.h"
#include "Exception.h"
#include "Engine.h"

namespace storm {
	namespace bs {

		Var::Var(Block *block, SrcName *type, syntax::SStr *name, Actuals *params) : Expr(name->pos) {
			init(block, block->scope.value(type), name);
			initTo(params, false);
		}

		Var::Var(Block *block, Value type, syntax::SStr *name, Actuals *params) : Expr(name->pos) {
			init(block, type.asRef(false), name);
			initTo(params, false);
		}

		Var::Var(Block *block, SrcName *type, syntax::SStr *name, Expr *init) : Expr(name->pos) {
			this->init(block, block->scope.value(type), name);
			initTo(init);
		}

		Var::Var(Block *block, Value type, syntax::SStr *name, Expr *init) : Expr(name->pos) {
			this->init(block, type.asRef(false), name);
			initTo(init);
		}

		Var::Var(Block *block, syntax::SStr *name, Expr *init) : Expr(name->pos) {
			this->init(block, init->result().type().asRef(false), name);
			// We don't need to call 'initTo' since we know that 'init' will work properly anyway!.
			initExpr = init;
		}

		void Var::init(Block *block, const Value &type, syntax::SStr *name) {
			var = new (this) LocalVar(name->v, type, name->pos, false);
			scope = block->scope;
			block->add(var);
		}

		void Var::initTo(Expr *e) {
			if (Expr *z = castTo(e, var->result, scope))
				initExpr = z;
			else
				// Use a ctor...
				initTo(new (this) Actuals(e), true);
		}

		void Var::initTo(Actuals *actuals, Bool castOnly) {
			if (var->result.isPrimitive()) {
				// Assignment is the same as initialization here...
				nat size = actuals->expressions->count();
				if (size == 1) {
					Expr *e = actuals->expressions->at(0);
					// Check types!
					if (e->result() == var->result) {
						initExpr = e;
						return;
					}
				} else if (size == 0) {
					// No constructor, initialized to zero!
					return;
				}
			}

			Type *t = var->result.type;
			BSNamePart *name = new (this) BSNamePart(Type::CTOR, pos, actuals);
			name->insert(thisPtr(t));
			Function *ctor = as<Function>(t->find(name, scope));
			if (!ctor) {
				Str *msg = TO_S(engine(), S("No appropriate constructor for ") << var->result
								<< S(" found. Can not initialize ") << var->name
								<< S(". Expected signature: ") << name);
				throw new (this) SyntaxError(var->pos, msg);
			}

			if (castOnly && !implicitlyCallableCtor(ctor)) {
				Str *msg = TO_S(engine(), S("The constructor ") << ctor->shortIdentifier()
								<< S(" is not a copy constructor or marked with 'cast', and")
								<< S(" therefore needs to be called explicitly. If this was")
								<< S(" your intention, use \"") << t->name << S(" ")
								<< var->name << S("(...);\" instead."));
				throw new (this) SyntaxError(var->pos, msg);
			}

			initCtor = new (this) CtorCall(pos, scope, ctor, actuals);
		}

		ExprResult Var::result() {
			return var->result.asRef();
		}

		void Var::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			const Value &t = var->result;

			if (t.isValue() && !t.isPrimitive()) {
				Expr *ctor = null;

				if (initCtor) {
					ctor = initCtor;
				} else if (initExpr) {
					// If the expression has a suitable type, we can avoid calling the copy
					// constructor and instead ask the expression to create the value in the correct
					// location directly. This is similar copy-ctor elison in C++.
					if (var->result.mayStore(initExpr->result().type())) {
						ctor = initExpr;
					} else {
						ctor = copyCtor(pos, scope, t.type, initExpr);
					}
				} else {
					ctor = defaultCtor(pos, scope, t.type);
				}

				if (ctor) {
					CodeResult *gr = new (this) CodeResult(var->result, var->var);
					ctor->code(s, gr);
				}
			} else if (initExpr) {
				CodeResult *gr = new (this) CodeResult(var->result, var->var);
				initExpr->code(s, gr);
			} else if (initCtor) {
				CodeResult *gr = new (this) CodeResult(var->result, var->var);
				initCtor->code(s, gr);
			}

			if (to->needed()) {
				// Part of another expression.
				code::Var v = to->location(s);
				Value toType = to->type();
				if (toType.ref) {
					*s->l << lea(v, var->var.v);
				} else if (!to->suggest(s, var->var.v)) {
					// Need to copy it:
					if (toType.isAsmType()) {
						*s->l << mov(v, var->var.v);
					} else {
						*s->l << lea(ptrA, v);
						*s->l << lea(ptrC, var->var.v);
						*s->l << fnParam(engine().ptrDesc(), ptrA);
						*s->l << fnParam(engine().ptrDesc(), ptrC);
						*s->l << fnCall(toType.copyCtor(), false);
					}
				}
				to->created(s);
			}
		}

		SrcPos Var::largePos() {
			SrcPos result = pos;
			if (initExpr)
				result = result.extend(initExpr->largePos());
			if (initCtor)
				result = result.extend(initCtor->largePos());
			return result;
		}

		void Var::toS(StrBuf *to) const {
			if (var->constant)
				*to << S("const ");
			*to << var->result << S(" ") << var->name;
			if (initExpr)
				*to << S(" = ") << initExpr;
			else if (initCtor)
				*to << S("(") << initCtor << S(")");
		}


		/**
		 * ThisVar
		 */

		ThisVar::ThisVar(Value val, SrcPos pos, Bool param)
			: LocalVar(new (engine()) Str(S("this")), val, pos, param) {
			constant = true;
		}

		ThisVar::ThisVar(Str *name, Value val, SrcPos pos, Bool param)
			: LocalVar(name, val, pos, param) {
			constant = true;
		}

		Bool ThisVar::thisVariable() {
			return true;
		}


		/**
		 * LocalVar
		 */

		LocalVar::LocalVar(Str *name, Value val, SrcPos pos, Bool param)
			: Named(name), result(val), var(), param(param), constant(false) {

			this->pos = pos;

			// Disallow 'void' variables.
			if (val == Value())
				throw new (this) SyntaxError(pos,
											TO_S(this, S("Attempted to create the variable \"")
												<< name << S("\" with type \"void\", which is not allowed.")));
		}

		Bool LocalVar::thisVariable() {
			return false;
		}

		void LocalVar::create(CodeGen *state) {
			if (param)
				return;

			if (var.v != code::Var()) {
				assert(state->l->accessible(var.v, state->block));
			} else {
				var = state->createVar(result);
				addInfo(state->l, var.v);
			}
		}

		void LocalVar::createParam(CodeGen *state) {
			using namespace code;

			if (!param)
				return;
			assert(var.v == code::Var(), L"Already created!");

			var = VarInfo(state->createParam(result));
			addInfo(state->l, var.v);
		}

		void LocalVar::addInfo(code::Listing *l, code::Var var) {
			if (result.type)
				l->varInfo(var, new (this) code::Listing::VarInfo(name, result.type, result.ref, pos));
		}


		LocalVar *createParam(EnginePtr e, ValParam param, SrcPos pos) {
			if (param.thisParam())
				return new (e.v) ThisVar(param.name, param.type(), pos, true);
			else
				return new (e.v) LocalVar(param.name, param.type(), pos, true);
		}

	}
}
