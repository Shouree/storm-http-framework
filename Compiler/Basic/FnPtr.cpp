#include "stdafx.h"
#include "FnPtr.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Fn.h"
#include "Compiler/Lib/Maybe.h"
#include "Core/Join.h"

namespace storm {
	namespace bs {

		static void checkDot(Expr *dotExpr) {
			Value dotResult = dotExpr->result().type();
			if (dotResult.isValue())
				throw new (dotExpr) SyntaxError(dotExpr->pos,
												S("Only classes and actors can be bound to a function pointer. Not values."));
		}

		FnPtr::FnPtr(Block *block, SrcName *name, Array<SrcName *> *formal) : Expr(name->pos) {
			findTarget(block->scope, name, formal, null);
		}

		FnPtr::FnPtr(Block *block, Expr *dot, syntax::SStr *name, Array<SrcName *>*formal)
			: Expr(name->pos), dotExpr(dot) {

			checkDot(dotExpr);

			SrcName *tn = new (this) SrcName(name->pos);
			tn->add(new (this) SimplePart(name->v));
			findTarget(block->scope, tn, formal, dot);
		}

		FnPtr::FnPtr(Block *block, SrcName *name) : Expr(name->pos), parent(block), name(name->simplify(block->scope)) {
			if (!this->name) {
				Str *msg = TO_S(engine(), S("The parameters of the name ") << name <<
								S(" can not be resolved to proper types."));
				throw new (this) SyntaxError(name->pos, msg);
			}
		}

		FnPtr::FnPtr(Block *block, Expr *dot, syntax::SStr *name) : Expr(name->pos), dotExpr(dot), parent(block) {
			checkDot(dotExpr);

			SimpleName *tn = new (this) SimpleName();
			tn->add(new (this) SimplePart(name->v));
			this->name = tn;
		}

		FnPtr::FnPtr(Function *target, SrcPos pos) : Expr(pos), target(target) {
			Array<Value> *formals = clone(target->params);
			formals->insert(0, target->result);
			ptrType = thisPtr(fnType(formals));
		}

		FnPtr::FnPtr(Expr *dot, Function *target, SrcPos pos) : Expr(pos), dotExpr(dot), target(target) {
			checkDot(dotExpr);

			if (target->params->empty() || !target->params->at(0).canStore(dot->result().type()))
				throw new (this) SyntaxError(dotExpr->pos, S("The first parameter of the specified function does not match the type of the provided expression."));

			Array<Value> *formals = clone(target->params);
			formals->at(0) = target->result;
			ptrType = thisPtr(fnType(formals));
		}

		// Note: We don't support implicit this pointer.
		void FnPtr::findTarget(const Scope &scope, SrcName *name, Array<SrcName *> *formal, MAYBE(Expr *) dot) {
			SimpleName *resolved = name->simplify(scope);
			if (!resolved) {
				Str *msg = TO_S(engine(), S("The parameters of the name ") << name
								<< S(" can not be resolved to proper types."));
				throw new (this) SyntaxError(name->pos, msg);
			}

			Array<Value> *params = new (this) Array<Value>();
			Array<Value> *formals = new (this) Array<Value>();
			if (dot)
				params->push(dot->result().type());
			for (Nat i = 0; i < formal->count(); i++) {
				Value v = scope.value(formal->at(i));
				params->push(v);
				formals->push(v);
			}

			SimplePart *last = new (this) SimplePart(resolved->last()->name, params);
			resolved->last() = last;

			Named *found = scope.find(resolved);
			if (!found)
				throw new (this) SyntaxError(name->pos, TO_S(engine(), S("Could not find ") << resolved));
			target = as<Function>(found);
			if (!target) {
				Str *msg = TO_S(engine(), S("Can not take the pointer of anything other than a function. This is a ")
								<< found << S("!"));
				throw new (this) SyntaxError(name->pos, msg);
			}

			formals->insert(0, target->result);
			ptrType = thisPtr(fnType(formals));
		}

		Function *FnPtr::acceptableFn(Value &t) {
			FnType *ptrType = as<FnType>(t.type);
			// Check if wrapped in a maybe type?
			if (!ptrType) {
				if (MaybeType *maybeType = as<MaybeType>(t.type)) {
					t = maybeType->param();
					ptrType = as<FnType>(t.type);
				}
			}
			if (!ptrType)
				return null;
			if (ptrType->params->empty())
				return null;

			Array<Value> *params = clone(ptrType->params);
			Value result = params->at(0);
			if (dotExpr)
				params->at(0) = dotExpr->result().type();
			else
				params->remove(0);

			SimplePart *last = new (this) SimplePart(this->name->last()->name, params);
			SimpleName *name = clone(this->name);
			name->last() = last;

			Function *found = as<Function>(parent->scope.find(name));
			if (!found)
				return null;
			if (!result.canStore(found->result))
				return null;

			return found;
		}

		ExprResult FnPtr::result() {
			return ptrType;
		}

		Int FnPtr::castPenalty(Value to) {
			if (ptrType != Value()) {
				// If types were specified, no automatic cast.
				return to.asRef(false) == ptrType ? 0 : -1;
			} else if (acceptableFn(to)) {
				return 10;
			} else {
				return -1;
			}
		}

		void FnPtr::code(CodeGen *to, CodeResult *r) {
			Value type = result().type();

			if (!type.type) {
				// Types were deduced automatically, we need to check some more...
				type = r->type();
				if (!type.type) {
					throw new (this) SyntaxError(pos, S("Unable to deduce parameter types for a function pointer ")
												S("in this context. Please specify parameter types explicitly."));
				}

				Function *target = acceptableFn(type);
				if (!target) {
					Str *msg = TO_S(engine(), S("Failed to find a suitable function for the function pointer type ")
									<< type << S(". Please specify explicit parameters."));
					throw new (this) SyntaxError(pos, msg);
				}

				code(to, r, type, target);
			} else {
				assert(target);
				code(to, r, type, target);
			}
		}

		void FnPtr::code(CodeGen *to, CodeResult *r, Value type, Function *target) {
			using namespace code;

			// Note: initialized to zero if not needed.
			VarInfo thisPtr = to->createVar(type);

			if (dotExpr) {
				CodeResult *result = new (this) CodeResult(type, thisPtr);
				dotExpr->code(to, result);
			}

			if (!r->needed())
				return;

			code::Var z = r->location(to);
			if (!dotExpr) {
				// Create the object once and store it.
				FnBase *obj = pointer(target);

				*to->l << mov(z, objPtr(obj));
			} else {
				// We need to create a new object each time since the 'dotExpr' might change.
				RunOn runOn = target->runOn();
				bool memberFn = target->isMember();
				code::TypeDesc *ptr = engine().ptrDesc();

				*to->l << lea(ptrA, target->ref());
				*to->l << fnParam(ptr, type.type->typeRef());
				*to->l << fnParam(ptr, ptrA);
				if (runOn.state == RunOn::named)
					*to->l << fnParam(ptr, runOn.thread->ref());
				else
					*to->l << fnParam(ptr, ptrConst(Offset()));
				*to->l << fnParam(ptr, thisPtr.v);
				*to->l << fnParam(byteDesc(engine()), byteConst(memberFn ? 1 : 0));
				*to->l << fnCall(engine().ref(builtin::fnCreate), false, ptr, ptrA);
				*to->l << mov(z, ptrA);
			}
			r->created(to);
		}

		void FnPtr::toS(StrBuf *to) const {
			*to << S("&");

			if (target && ptrType.type) {
				if (dotExpr) {
					*to << dotExpr << S(".") << target->name;
				} else {
					SimpleName *p = target->path();
					p->last()->params = new (this) Array<Value>();
					*to << p;
				}

				*to << S("(");
				for (Nat i = 1; i < ptrType.type->params->count(); i++) {
					if (i != 1)
						*to << S(", ");
					*to << ptrType.type->params->at(i);
				}
				*to << S(")");
			} else if (name) {
				*to << name;
			} else {
				*to << S("<invalid>");
			}
		}

	}
}
