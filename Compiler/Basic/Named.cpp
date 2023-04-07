#include "stdafx.h"
#include "Named.h"
#include "Exception.h"
#include "Cast.h"
#include "Lookup.h"
#include "Actuals.h"
#include "Block.h"
#include "Resolve.h"
#include "Compiler/Engine.h"
#include "Compiler/Lib/Maybe.h"
#include "Compiler/Lib/Future.h"

namespace storm {
	namespace bs {

		static void callOnThread(Function *fn, CodeGen *s, Array<code::Operand> *actuals,
								CodeResult *to, bool lookup, bool sameObject) {

			if (sameObject) {
				// Regular function call is enough.
				fn->localCall(s, actuals, to, lookup);
			} else {
				fn->autoCall(s, actuals, to);
			}
		}

		// Call the function and handle the result correctly.
		static void callFn(Function *call, CodeGen *s, Array<code::Operand> *values,
						CodeResult *to, bool lookup, bool sameObject) {
			using namespace code;

			Engine &e = call->engine();
			if (!to->needed() || to->type().ref == call->result.ref) {
				callOnThread(call, s, values, to, lookup, sameObject);
				return;
			}

			// We need to do stuff!
			CodeResult *t = new (call) CodeResult(call->result, s->block);
			callOnThread(call, s, values, t, lookup, sameObject);

			if (!to->needed())
				return;

			code::Var r = to->location(s);
			if (to->type().ref) {
				// Dangerous...
				*s->l << lea(r, ptrRel(t->location(s), Offset()));
			} else if (!to->type().isAsmType()) {
				// Need to copy...
				*s->l << lea(ptrA, ptrRel(r, Offset()));
				*s->l << fnParam(e.ptrDesc(), ptrA);
				*s->l << fnParam(e.ptrDesc(), t->location(s));
				*s->l << fnCall(to->type().copyCtor(), true);
			} else {
				// Regular machine operations suffice!
				*s->l << mov(ptrA, t->location(s));
				*s->l << mov(r, xRel(to->type().size(), ptrA, Offset()));
			}
			to->created(s);
		}

		// Call async function.
		static void callAsyncFn(Function *fn, CodeGen *s, Array<code::Operand> *actuals, CodeResult *to, bool sameObject) {
			if (sameObject) {
				fn->asyncLocalCall(s, actuals, to);
			} else {
				fn->asyncAutoCall(s, actuals, to);
			}
		}


		/**
		 * Function call.
		 */

		FnCall::FnCall(SrcPos pos, Scope scope, Function *toExecute, Actuals *params, Bool lookup)
			: Expr(pos), toExecute(toExecute), params(params), scope(scope), lookup(lookup), async(false) {

			if (params->expressions->count() != toExecute->params->count())
				throw new (this) SyntaxError(pos, S("The parameter count does not match!"));
		}

		FnCall::FnCall(SrcPos pos, Scope scope, Function *toExecute, Actuals *params)
			: Expr(pos), toExecute(toExecute), params(params), scope(scope), lookup(true), async(false) {

			if (params->expressions->count() != toExecute->params->count())
				throw new (this) SyntaxError(pos, S("The parameter count does not match!"));
		}

		UnresolvedName *FnCall::name() {
			// This is approximate, and assumes that if we have a getter, the setter must be located
			// at the same place (which is actually fairly reasonable in many cases). This is,
			// however, not always true.

			BlockLookup *lookup = as<BlockLookup>(scope.top);
			if (!lookup)
				throw new (this) InternalError(S("Can not use FnCall::name() without having a scope referring to a block."));
			SimpleName *path = toExecute->path();
			path->last()->params = new (this) Array<Value>();
			return new (this) UnresolvedName(lookup->block, path, pos, params, false);
		}

		void FnCall::makeAsync() {
			async = true;
		}

		ExprResult FnCall::result() {
			if (async)
				return wrapFuture(engine(), toExecute->result);
			else
				return toExecute->result;
		}

		void FnCall::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			// Execute on the same thread?
			Bool sameObject = params->hasThisFirst();

			// Note that toExecute->params may not be equal to params->values()
			// since some actual parameters may report that they can return a
			// reference to a value. However, we know that toExecute->params.canStore(params->values())
			// so it is OK to take toExecute->params directly.
			Array<Value> *values = toExecute->params;
			Array<code::Operand> *vars = new (this) Array<code::Operand>(values->count());

			// Load parameters.
			for (nat i = 0; i < values->count(); i++)
				vars->at(i) = params->code(i, s, values->at(i), scope);

			// Call!
			if (async) {
				if (!lookup)
					throw new (this) SyntaxError(pos, S("Can not use 'spawn' with super-calls."));
				callAsyncFn(toExecute, s, vars, to, sameObject);
			} else {
				callFn(toExecute, s, vars, to, lookup, sameObject);
			}
		}

		SrcPos FnCall::largePos() {
			SrcPos result = pos;
			for (Nat i = 0; i < params->expressions->count(); i++)
				result = result.extend(params->expressions->at(i)->largePos());
			result.end++;
			return result;
		}

		void FnCall::toS(StrBuf *to) const {
			SimpleName *p = toExecute->safePath();
			p->last() = new (p) SimplePart(p->last()->name);
			*to << p << params;
			if (async)
				*to << S("&");
		}

		/**
		 * Execute constructor.
		 */

		CtorCall::CtorCall(SrcPos pos, Scope scope, Function *ctor, Actuals *params)
			: Expr(pos), ctor(ctor), params(params), scope(scope) {

			assert(params->expressions->count() == ctor->params->count() - 1, L"Invalid number of parameters to constructor!");
			toCreate = ctor->params->at(0).asRef(false);

			// Throws an error if we should not instantiate the type.
			toCreate.type->ensureNonAbstract(pos);
		}

		ExprResult CtorCall::result() {
			return toCreate.asRef(false);
		}

		void CtorCall::toS(StrBuf *to) const {
			*to << toCreate << params;
		}

		void CtorCall::code(CodeGen *s, CodeResult *to) {
			if (toCreate.isValue())
				createValue(s, to);
			else
				createClass(s, to);
		}

		void CtorCall::createValue(CodeGen *s, CodeResult *to) {
			using namespace code;
			Engine &e = engine();

			// Call the constructor with the address of our variable.
			Array<Value> *values = ctor->params;
			Array<code::Operand> *vars = new (e) Array<code::Operand>(values->count());

			// Load parameters.
			for (nat i = 1; i < values->count(); i++)
				vars->at(i) = params->code(i - 1, s, values->at(i), scope);

			// This needs to be last, otherwise other generated code may overwrite it!
			code::Var thisVar;
			if (to->type().ref) {
				thisVar = to->location(s);
				vars->at(0) = thisVar;
			} else {
				thisVar = to->safeLocation(s, toCreate.asRef(false));
				vars->at(0) = code::Operand(ptrA);
				*s->l << lea(vars->at(0), thisVar);
			}

			// Call it!
			CodeResult *voidTo = new (this) CodeResult(Value(), s->block);
			ctor->localCall(s, vars, voidTo, false);
			to->created(s);
		}

		void CtorCall::createClass(CodeGen *s, CodeResult *to) {
			using namespace code;
			Engine &e = engine();

			Array<Value> *values = ctor->params;
			Array<code::Operand> *vars = new (e) Array<code::Operand>(values->count() - 1);

			for (nat i = 0; i < values->count() - 1; i++)
				vars->at(i) = params->code(i, s, values->at(i + 1), scope);

			code::Var created = to->safeLocation(s, toCreate);
			allocObject(s, ctor, vars, created);
			to->created(s);
		}

		SrcPos CtorCall::largePos() {
			SrcPos result = pos;
			for (Nat i = 0; i < params->expressions->count(); i++)
				result = result.extend(params->expressions->at(i)->largePos());
			result.end++;
			return result;
		}

		CtorCall *defaultCtor(const SrcPos &pos, Scope scope, Type *t) {
			Function *f = t->defaultCtor();
			if (!f)
				throw new (t) SyntaxError(pos, TO_S(t, S("No default constructor for ") << t->identifier()));

			Actuals *actual = new (t) Actuals();
			return new (t) CtorCall(pos, scope, f, actual);
		}

		CtorCall *copyCtor(const SrcPos &pos, Scope scope, Type *t, Expr *src) {
			Function *f = t->copyCtor();
			if (!f)
				throw new (t) SyntaxError(pos, TO_S(t, S("No copy-constructor for ") << t->identifier()));

			Actuals *actual = new (t) Actuals();
			actual->add(src);
			return new (t) CtorCall(pos, scope, f, actual);
		}


		/**
		 * Local variable.
		 */
		LocalVarAccess::LocalVarAccess(SrcPos pos, LocalVar *var) : Expr(pos), var(var) {}

		ExprResult LocalVarAccess::result() {
			if (var->constant)
				return var->result;
			else
				return var->result.asRef();
		}

		void LocalVarAccess::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			if (!to->needed())
				return;

			if (to->type().ref && !var->result.ref) {
				code::Var v = to->location(s);
				*s->l << lea(v, var->var.v);
				to->created(s);
			} else if (!to->type().ref && var->result.ref) {
				// Convert from "reference" to "plain type"
				code::Var v = to->location(s);
				if (!var->result.isAsmType()) {
					*s->l << lea(ptrC, v);
					*s->l << fnParam(engine().ptrDesc(), ptrC);
					*s->l << fnParam(engine().ptrDesc(), var->var.v);
					*s->l << fnCall(var->result.copyCtor(), true);
				} else {
					*s->l << mov(ptrA, var->var.v);
					*s->l << mov(v, xRel(v.size(), ptrA, Offset()));
				}
				to->created(s);

			} else if (!to->suggest(s, var->var.v)) {

				code::Var v = to->location(s);
				if (to->type().ref) {
					// Both are references
					*s->l << mov(v, var->var.v);
				} else if (!var->result.isAsmType()) {
					*s->l << lea(ptrA, var->var.v);
					*s->l << lea(ptrC, v);
					*s->l << fnParam(engine().ptrDesc(), ptrC);
					*s->l << fnParam(engine().ptrDesc(), ptrA);
					*s->l << fnCall(var->result.copyCtor(), true);
				} else {
					*s->l << mov(v, var->var.v);
				}
				to->created(s);
			}
		}

		void LocalVarAccess::toS(StrBuf *to) const {
			*to << var->name;
		}

		/**
		 * Bare local variable.
		 */
		BareVarAccess::BareVarAccess(SrcPos pos, Value type, code::Var var) : Expr(pos), type(type), var(var) {}

		ExprResult BareVarAccess::result() {
			return type;
		}

		void BareVarAccess::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			if (!to->needed())
				return;

			if (to->type().ref && !type.ref) {
				code::Var v = to->location(s);
				*s->l << lea(v, var);
				to->created(s);
			} else if (!to->type().ref && type.ref) {
				// Convert from "reference" to "plain type"
				code::Var v = to->location(s);
				if (!type.isAsmType()) {
					*s->l << lea(ptrC, v);
					*s->l << fnParam(engine().ptrDesc(), ptrC);
					*s->l << fnParam(engine().ptrDesc(), var);
					*s->l << fnCall(type.copyCtor(), true);
				} else {
					*s->l << mov(ptrA, var);
					*s->l << mov(v, xRel(v.size(), ptrA, Offset()));
				}
				to->created(s);

			} else if (!to->suggest(s, var)) {

				code::Var v = to->location(s);
				if (to->type().ref) {
					// Both are references
					*s->l << mov(v, var);
				} else if (!type.isAsmType()) {
					*s->l << lea(ptrA, var);
					*s->l << lea(ptrC, v);
					*s->l << fnParam(engine().ptrDesc(), ptrC);
					*s->l << fnParam(engine().ptrDesc(), ptrA);
					*s->l << fnCall(type.copyCtor(), true);
				} else {
					*s->l << mov(v, var);
				}
				to->created(s);
			}
		}

		void BareVarAccess::toS(StrBuf *to) const {
			*to << type << S("<bare>") << code::Operand(var);
		}


		/**
		 * Member variable.
		 */

		MemberVarAccess::MemberVarAccess(SrcPos pos, Expr *member, MemberVar *var)
			: Expr(pos), member(member), var(var), assignTo(false), implicit(false) {}

		MemberVarAccess::MemberVarAccess(SrcPos pos, Expr *member, MemberVar *var, Bool implicit)
			: Expr(pos), member(member), var(var), assignTo(false), implicit(implicit) {}

		ExprResult MemberVarAccess::result() {
			return var->type.asRef();
		}

		void MemberVarAccess::assignResult() {
			assignTo = true;
		}

		SrcPos MemberVarAccess::largePos() {
			return pos.extend(member->pos);
		}

		void MemberVarAccess::code(CodeGen *s, CodeResult *to) {
			if (!to->needed()) {
				// We still need to evaluate 'member', it may have side effects!
				CodeResult *mResult = CREATE(CodeResult, this);
				member->code(s, mResult);
				return;
			}

			if (var->owner()->typeFlags & typeValue) {
				valueCode(s, to);
			} else {
				classCode(s, to);
			}
		}

		void MemberVarAccess::valueCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			Value mType = member->result().type();
			code::Var memberPtr;

			{
				CodeResult *mResult = new (this) CodeResult(mType, s->block);
				member->code(s, mResult);
				memberPtr = mResult->location(s);
			}

			// If it was a reference, we can use it right away!
			if (mType.ref) {
				*s->l << mov(ptrA, memberPtr);
			} else {
				*s->l << lea(ptrA, memberPtr);
			}

			extractCode(s, to);
		}

		void MemberVarAccess::classCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			CodeResult *mResult = new (this) CodeResult(member->result().type().asRef(false), s->block);
			member->code(s, mResult);
			*s->l << mov(ptrA, mResult->location(s));

			extractCode(s, to);
		}

		void MemberVarAccess::extractCode(CodeGen *s, CodeResult *to) {
			RunOn target = var->owner()->runOn();
			if (member->thisVariable() || s->runOn.canRun(target)) {
				extractPlainCode(s, to);
				return;
			}

			// We need to copy. See if we shall warn the user...
			if (assignTo)
				throw new (this) SyntaxError(pos, S("Unable to assign to member variables in objects running ")
											S("on a different thread than the caller. Use assignment functions ")
											S("for this task instead."));

			extractCopyCode(s, to);
		}

		void MemberVarAccess::extractPlainCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			code::Var result = to->location(s);
			if (to->type().ref) {
				*s->l << add(ptrA, ptrConst(var->offset()));
				*s->l << mov(result, ptrA);
			} else if (!to->type().isAsmType()) {
				*s->l << add(ptrA, ptrConst(var->offset()));
				*s->l << lea(ptrC, result);
				*s->l << fnParam(engine().ptrDesc(), ptrC);
				*s->l << fnParam(engine().ptrDesc(), ptrA);
				*s->l << fnCall(var->type.copyCtor(), true);
			} else {
				*s->l << mov(result, xRel(result.size(), ptrA, var->offset()));
			}

			to->created(s);
		}

		void MemberVarAccess::extractCopyCode(CodeGen *s, CodeResult *to) {
			using namespace code;

			Value type = to->type();
			code::Var result = to->location(s);
			VarInfo local(result, false);
			if (type.ref) {
				// We need to create a value here that we can use later.
				local = s->createVar(type.asRef(false));
			}

			Function *fn = type.type->readRefFn();

			// Figure out which thread is the proper one. We can not use Function::findThread since
			// that would find the thread for the *function* rather than the object we're reading from.
			code::Var thread = s->l->createVar(s->block, Size::sPtr);
			RunOn runOn = var->owner()->runOn();
			switch (runOn.state) {
			case RunOn::runtime:
				*s->l << mov(ptrC, ptrA);
				*s->l << add(ptrC, engine().ref(builtin::TObjectOffset));
				*s->l << mov(thread, ptrRel(ptrC, Offset()));
				break;
			case RunOn::named:
				*s->l << mov(thread, runOn.thread->ref());
				break;
			default:
				throw new (this) InternalError(S("Can not issue a threaded read on a non-threaded object."));
			}

			// Offset the pointer and continue. Note: We do not want to modify 'obj' since that
			// could be used as eg. a local variable.
			code::Var data = s->l->createVar(s->block, Size::sPtr);
			*s->l << add(ptrA, ptrConst(var->offset()));
			*s->l << mov(data, ptrA);

			// Call the function on the proper thread.
			Array<Operand> *params = new (this) Array<Operand>(1, Operand(data));
			fn->threadCall(s, params, new (this) CodeResult(type.asRef(false), local), thread);

			if (type.ref) {
				*s->l << lea(result, local.v);
			}
			to->created(s);
		}

		void MemberVarAccess::toS(StrBuf *to) const {
			*to << member << S(".") << var->name;
		}


		/**
		 * Global variable.
		 */

		GlobalVarAccess::GlobalVarAccess(SrcPos pos, GlobalVar *var) : Expr(pos), var(var) {}

		ExprResult GlobalVarAccess::result() {
			return var->type.asRef();
		}

		void GlobalVarAccess::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			// Check so that we're using the proper thread!
			if (!var->accessibleFrom(s->runOn)) {
				Str *msg = TO_S(engine(), S("Can not access the global variable ") << var
								<< S(" from a thread other than ") << var->owner);
				throw new (this) SyntaxError(pos, msg);
			}

			if (!to->needed())
				return;


			*s->l << fnParam(engine().ptrDesc(), objPtr(var));
			*s->l << fnCall(engine().ref(builtin::globalAddr), true, engine().ptrDesc(), ptrA);

			if (to->type().ref) {
				*s->l << mov(to->location(s), ptrA);
			} else {
				// We need to make a copy...
				code::Var d = to->location(s);
				if (!var->type.isAsmType()) {
					*s->l << lea(ptrC, d);
					*s->l << fnParam(engine().ptrDesc(), ptrC);
					*s->l << fnParam(engine().ptrDesc(), ptrA);
					*s->l << fnCall(var->type.copyCtor(), true);
				} else {
					*s->l << mov(d, xRel(d.size(), ptrA, Offset()));
				}
				to->created(s);
			}
		}

		void GlobalVarAccess::toS(StrBuf *to) const {
			*to << var->identifier();
		}


		/**
		 * Named thread.
		 */

		NamedThreadAccess::NamedThreadAccess(SrcPos pos, NamedThread *thread) : Expr(pos), thread(thread) {}

		ExprResult NamedThreadAccess::result() {
			return Value(Thread::stormType(engine()));
		}

		void NamedThreadAccess::code(CodeGen *s, CodeResult *to) {
			if (to->needed()) {
				code::Var z = to->location(s);
				*s->l << code::mov(z, thread->ref());
				to->created(s);
			}
		}

		void NamedThreadAccess::toS(StrBuf *to) const {
			*to << thread->name;
		}


		/**
		 * Assignment.
		 */

		ClassAssign::ClassAssign(Expr *to, Expr *value, Scope scope) : Expr(to->pos), to(to) {
			Value r = to->result().type();
			if ((r.type->typeFlags & typeClass) != typeClass)
				throw new (this) TypeError(to->pos, S("The default assignment can not be used with other types than")
										S(" classes at the moment. Please implement an assignment operator for your type."));
			if (!r.ref)
				throw new (this) TypeError(to->pos, S("Can not assign to a non-reference."));

			this->value = castTo(value, r.asRef(false), scope);
			if (!this->value) {
				Str *msg = TO_S(engine(), S("Can not store a ") << value->result() << S(" in ") << r);
				throw new (this) TypeError(to->pos, msg);
			}
		}

		ExprResult ClassAssign::result() {
			return to->result().type().asRef(false);
		}

		void ClassAssign::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			// Type to work with.
			Value t = this->to->result().type().asRef(false);

			// Target variable.
			CodeResult *lhs = new (this) CodeResult(t.asRef(true), s->block);
			this->to->code(s, lhs);
			code::Operand targetAddr = lhs->location(s);

			// Compute RHS...
			CodeResult *rhs = new (this) CodeResult(t, s->block);
			value->code(s, rhs);

			// Copy.
			*s->l << code::mov(ptrA, targetAddr);
			*s->l << code::mov(ptrRel(ptrA, Offset()), rhs->location(s));

			// Do we need to return some value?
			if (!to->needed())
				return;

			if (to->type().ref) {
				if (!to->suggest(s, targetAddr)) {
					code::Var z = to->location(s);
					*s->l << mov(z, targetAddr);
					to->created(s);
				}
			} else {
				code::Var z = to->location(s);
				*s->l << mov(ptrA, targetAddr);
				*s->l << mov(z, ptrRel(ptrA, Offset()));
				to->created(s);
			}
		}

		void ClassAssign::toS(StrBuf *to) const {
			*to << to << S(" = ") << value;
		}


		/**
		 * Comparison.
		 */

		ClassCompare::ClassCompare(SrcPos pos, Expr *lhs, Expr *rhs, Bool negate) :
			Expr(pos), lhs(lhs), rhs(rhs), negate(negate) {

			Value l = lhs->result().type();
			Value r = rhs->result().type();

			l = unwrapMaybe(l);
			r = unwrapMaybe(r);

			SrcPos errorPos;
			if (!l.isObject())
				errorPos = lhs->pos;
			else if (!r.isObject())
				errorPos = rhs->pos;

			if (errorPos.any()) {
				Str *msg = TO_S(engine(), S("The default comparison operator can only be used with classes and actors.")
								S(" Found: ") << l << S(" and ") << r);
				throw new (this) TypeError(errorPos, msg);
			}

			if (!r.type->isA(l.type) && !l.type->isA(r.type))
				throw new (this) TypeError(lhs->pos, S("The left- and right-hand types are unrelated."));
		}

		ExprResult ClassCompare::result() {
			return ExprResult(Value(StormInfo<Bool>::type(engine())));
		}

		void ClassCompare::code(CodeGen *s, CodeResult *to) {
			using namespace code;

			Bool needed = to->needed();

			CodeResult *l = needed
				? new (this) CodeResult(lhs->result().type().asRef(false), s->block)
				: new (this) CodeResult();
			lhs->code(s, l);

			CodeResult *r = needed
				? new (this) CodeResult(rhs->result().type().asRef(false), s->block)
				: new (this) CodeResult();
			rhs->code(s, r);

			// Is our result needed?
			if (!to->needed())
				return;

			code::Var lVar = l->location(s);
			code::Var rVar = r->location(s);
			code::Var res = to->location(s);

			*s->l << cmp(lVar, rVar);
			*s->l << setCond(res, negate ? ifNotEqual : ifEqual);
			to->created(s);
		}

		void ClassCompare::toS(StrBuf *to) const {
			*to << lhs << S(" is ") << rhs;
		}


		Expr *STORM_FN spawnExpr(Expr *expr) {
			// If it is an unresolved name: poke it so that it throws the more appropriate error
			// rather than us delivering a less informative error.
			if (as<UnresolvedName>(expr))
				expr->result();

			FnCall *fnCall = as<FnCall>(expr);
			if (!fnCall) {
				Str *msg = TO_S(expr, S("The spawn-syntax is not applicable to anything but functions")
								S(" at the moment. This is a ") << runtime::typeOf(expr)->identifier());
				throw new (expr) SyntaxError(expr->pos, msg);
			}

			fnCall->makeAsync();

			return expr;
		}


	}
}
