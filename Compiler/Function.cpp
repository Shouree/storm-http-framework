#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"
#include "Exception.h"
#include "Code.h"
#include "Core/Str.h"
#include "Lib/Future.h"
#include "Lib/Clone.h"

namespace storm {

	Function::Function(Value result, Str *name, Array<Value> *params) :
		Named(name, params),
		result(result),
		lookupRef(null),
		codeRef(null),
		runOnThread(null),
		myFlags(fnNone) {}

	Function::Function(SrcPos pos, Value result, Str *name, Array<Value> *params) :
		Named(name, params),
		result(result),
		lookupRef(null),
		codeRef(null),
		runOnThread(null),
		myFlags(fnNone) {

		this->pos = pos;
	}


	const void *Function::pointer() {
		return ref().address();
	}

	code::Ref Function::ref() {
		initRefs();
		return code::Ref(lookupRef);
	}

	code::Ref Function::directRef() {
		initRefs();
		return code::Ref(codeRef);
	}

	Bool Function::isMember() {
		if (fnFlags() & fnStatic)
			return false;

		if (params->empty())
			return false;

		Type *first = params->at(0).type;
		return first == parent();
	}

	RunOn Function::runOn() const {
		RunOn result;
		if (Type *t = as<Type>(parent())) {
			result = t->runOn();
		}

		if (result.state != RunOn::any)
			return result;

		// The function itself can override if the parent class says "run on any thread".
		if (runOnThread) {
			return RunOn(runOnThread);
		} else {
			return RunOn(RunOn::any);
		}
	}

	void Function::runOn(NamedThread *t) {
		runOnThread = t;
	}

	Bool Function::pure() const {
		return (myFlags & fnPure) != 0;
	}

	Function *Function::make(FnFlags flag) {
		myFlags |= flag;
		return this;
	}

	FnFlags Function::fnFlags() const {
		return myFlags;
	}

	void Function::toS(StrBuf *to) const {
		*to << result << S(" ");
		Named::toS(to);

		if (myFlags != fnNone) {
			*to << S(" :");
			if (myFlags & fnPure)
				*to << S(" pure");
			if (myFlags & fnAutoCast)
				*to << S(" cast");
			if (myFlags & fnAssign)
				*to << S(" assign");
			if (myFlags & fnFinal)
				*to << S(" final");
			if (myFlags & fnAbstract)
				*to << S(" abstract");
			if (myFlags & fnOverride)
				*to << S(" override");
			if (myFlags & fnStatic)
				*to << S(" static");
		}
	}

	void Function::setCode(Code *code) {
		if (this->code)
			this->code->detach();
		code->attach(this);
		this->code = code;
		if (codeRef)
			this->code->update(codeRef);
	}

	Code *Function::getCode() const {
		return code;
	}

	void Function::setLookup(MAYBE(Code *) code) {
		if (lookup)
			lookup->detach();

		if (code == null && codeRef != null)
			lookup = new (this) DelegatedCode(code::Ref(codeRef));
		else
			lookup = code;

		if (lookup) {
			lookup->attach(this);
			if (lookupRef)
				lookup->update(lookupRef);
		}
	}

	Code *Function::getLookup() const {
		return lookup;
	}

	void Function::compile() {
		if (code)
			code->compile();
		if (lookup)
			lookup->compile();
	}

	void Function::discardSource() {
		if (GeneratedCode *c = as<GeneratedCode>(code))
			c->discardSource();
		if (GeneratedCode *l = as<GeneratedCode>(lookup))
			l->discardSource();
	}

	const void *Function::originalPtr() {
		return directRef().address();
	}

	void Function::initRefs() {
		if (!codeRef) {
			codeRef = new (this) NamedSource(this, Char('d'));
			if (code)
				code->update(codeRef);
		}

		if (!lookupRef) {
			lookupRef = new (this) NamedSource(this);
			if (!lookup) {
				lookup = new (this) DelegatedCode(code::Ref(codeRef));
				lookup->attach(this);
			}
			lookup->update(lookupRef);
		}
	}

	/**
	 * Function calls.
	 */

	code::Var Function::findThread(CodeGen *s, Array<code::Operand> *params) {
		using namespace code;

		RunOn on = runOn();
		assert(on.state != RunOn::any, L"Only use 'findThread' on functions which 'runOn()' return something other than any.");

		Var r = s->l->createVar(s->block, Size::sPtr);

		switch (on.state) {
		case RunOn::runtime:
			// Should be a this-ptr. Does not work well for constructors.
			assert(*name != Type::CTOR,
				L"Please overload 'findThread' for your constructor '" + ::toS(identifier()) + L"'!");
			*s->l << mov(ptrA, params->at(0));
			*s->l << add(ptrA, engine().ref(builtin::TObjectOffset));
			*s->l << mov(r, ptrRel(ptrA, Offset()));
			break;
		case RunOn::named:
			*s->l << mov(r, on.thread->ref());
			break;
		default:
			assert(false, L"Unknown state.");
			break;
		}

		return r;
	}

	void Function::autoCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result) {
		RunOn r = runOn();
		if (to->runOn.canRun(r)) {
			localCall(to, params, result, true);
		} else {
			code::Var v = findThread(to, params);
			threadCall(to, params, result, v);
		}
	}

	void Function::autoCallRef(CodeGen *to, Array<code::Operand> *params, code::Operand result) {
		RunOn r = runOn();
		if (to->runOn.canRun(r)) {
			localCallRef(to, params, result, true);
		} else {
			code::Var v = findThread(to, params);
			threadCallRef(to, params, result, v);
		}
	}

	void Function::localCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, Bool useLookup) {
		initRefs();
		if (params->count() != this->params->count()) {
			Str *msg = TO_S(engine(), S("Parameter count mismatch when calling ") << identifier()
							<< S(". Got ") << params->count() << S(" parameter(s)."));
			throw new (this) InternalError(msg);
		}

		InlineCode *inlined = as<InlineCode>(code);
		// If we're not going to use the lookup, we may choose to inline sooner.
		if (useLookup && as<DelegatedCode>(lookup) == null)
			inlined = null;

		if (inlined) {
			// TODO: We might want to create a new block here.
			inlined->code(to, params, result);
		} else {
			localCall(to, params, result, useLookup ? this->ref() : directRef());
		}
	}

	void Function::localCallRef(CodeGen *to, Array<code::Operand> *params, code::Operand result, Bool useLookup) {
		initRefs();
		if (params->count() != this->params->count()) {
			Str *msg = TO_S(engine(), S("Parameter count mismatch when calling ") << identifier()
							<< S(". Got ") << params->count() << S(" parameter(s)."));
			throw new (this) InternalError(msg);
		}

		InlineCode *inlined = as<InlineCode>(code);
		// If we're not going to use the lookup, we may choose to inline sooner.
		if (useLookup && as<DelegatedCode>(lookup) == null)
			inlined = null;

		if (inlined) {
			if (this->result.isAsmType()) {
				using namespace code;

				// TODO: We might want to create a new block here.
				CodeResult *r = new (this) CodeResult(this->result, to->block);
				inlined->code(to, params, r);
				*to->l << mov(ptrA, result);

				code::Var rVar = r->location(to);
				*to->l << mov(xRel(rVar.size(), ptrA, Offset()), rVar);
			} else {
				// Fall back on non-inlined version if we can't copy the result in-place.
				localCallRef(to, params, result, useLookup ? this->ref() : directRef());
			}
		} else {
			localCallRef(to, params, result, useLookup ? this->ref() : directRef());
		}
	}

	void Function::localCall(CodeGen *to, Array<code::Operand> *params, CodeResult *res, code::Ref ref) {
		using namespace code;

		addParams(to, params);

		if (result == Value()) {
			*to->l << fnCall(ref, isMember());
		} else {
			code::Var rVar = res->safeLocation(to, result);
			*to->l << fnCall(ref, isMember(), result.desc(engine()), rVar);
			res->created(to);
		}
	}

	void Function::localCallRef(CodeGen *to, Array<code::Operand> *params, code::Operand res, code::Ref ref) {
		using namespace code;

		addParams(to, params);

		if (result == Value()) {
			*to->l << fnCall(ref, isMember());
		} else {
			*to->l << fnCallRef(ref, isMember(), result.desc(engine()), res);
		}
	}

	void Function::addParams(CodeGen *to, Array<code::Operand> *params) {
		for (nat i = 0; i < params->count(); i++) {
			addParam(to, params, i);
		}
	}

	void Function::addParam(CodeGen *to, Array<code::Operand> *params, nat id) {
		Value p = this->params->at(id);
		if (!p.ref && p.isValue()) {
			if (params->at(id).type() == code::opVariable) {
				*to->l << fnParam(p.desc(engine()), params->at(id).var());
			} else {
				// Has to be a reference to the value...
				*to->l << fnParamRef(p.desc(engine()), params->at(id));
			}
		} else {
			*to->l << fnParam(p.desc(engine()), params->at(id));
		}
	}

	static inline code::Operand toOp(bool v) {
		return code::byteConst(v ? 1 : 0);
	}

	void Function::threadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *res, code::Operand thread) {
		using namespace code;

		Engine &e = engine();
		CodeGen *sub = to->child();
		*to->l << begin(sub->block);

		// Spill any registers to memory if necessary...
		params = spillRegisters(sub, params);

		// Create the parameters.
		Var par = createFnCall(sub, this->params, params, true);

		// Where shall we store the result (store the pointer to it in ptrB)?
		code::Var resultPos;
		if (this->result == Value()) {
			// null-pointer.
			*to->l << mov(ptrB, ptrConst(Offset(0)));
		} else {
			// Note: We always create the result in the parent scope if we need to, otherwise we
			// will fail to clone it later on.
			resultPos = res->safeLocation(to, this->result);
			*to->l << lea(ptrB, resultPos);
		}

		Ref thunk = Ref(threadThunk());

		TypeDesc *ptr = e.ptrDesc();

		// Spawn the thread!
		*to->l << lea(ptrA, par);
		*to->l << fnParam(ptr, ref()); // fn
		*to->l << fnParam(byteDesc(e), toOp(isMember())); // member
		*to->l << fnParam(ptr, thunk); // thunk
		*to->l << fnParam(ptr, ptrA); // params
		*to->l << fnParam(ptr, ptrB); // result
		*to->l << fnParam(ptr, thread); // on
		*to->l << fnCall(e.ref(builtin::spawnResult), false);

		*to->l << end(sub->block);

		if (resultPos != code::Var())
			res->created(to);

		// Clone the result.
		if (result.isValue() && !result.isPrimitive()) {
			if (Function *f = result.type->deepCopyFn()) {
				CodeGen *child = to->child();
				*to->l << begin(child->block);

				Var env = allocObject(child, CloneEnv::stormType(e));
				*to->l << lea(ptrA, resultPos);
				*to->l << fnParam(ptr, ptrA);
				*to->l << fnParam(ptr, env);
				*to->l << fnCall(f->ref(), true);

				*to->l << end(child->block);
			}
		} else if (result.isClass()) {
			*to->l << fnParam(ptr, resultPos);
			*to->l << fnCall(cloneFn(result.type)->ref(), false, ptr, resultPos);
		}

	}

	void Function::threadCallRef(CodeGen *to, Array<code::Operand> *params, code::Operand res, code::Operand thread) {
		using namespace code;

		Engine &e = engine();
		CodeGen *sub = to->child();
		*to->l << begin(sub->block);

		// Spill any registers to memory if necessary...
		params = spillRegisters(sub, params);

		// Create the parameters.
		Var par = createFnCall(sub, this->params, params, true);

		Ref thunk = Ref(threadThunk());

		TypeDesc *ptr = e.ptrDesc();

		// Spawn the thread!
		*to->l << lea(ptrA, par);
		*to->l << fnParam(ptr, ref()); // fn
		*to->l << fnParam(byteDesc(e), toOp(isMember())); // member
		*to->l << fnParam(ptr, thunk); // thunk
		*to->l << fnParam(ptr, ptrA); // params
		*to->l << fnParam(ptr, res); // result
		*to->l << fnParam(ptr, thread); // on
		*to->l << fnCall(e.ref(builtin::spawnResult), false);

		*to->l << end(sub->block);

		// Clone the result.
		if (result.isValue() && !result.isPrimitive()) {
			if (Function *f = result.type->deepCopyFn()) {
				CodeGen *child = to->child();
				*to->l << begin(child->block);

				Var env = allocObject(child, CloneEnv::stormType(e));
				*to->l << fnParam(ptr, res);
				*to->l << fnParam(ptr, env);
				*to->l << fnCall(f->ref(), true);

				*to->l << end(child->block);
			}
		} else if (result.isClass()) {
			*to->l << mov(ptrA, res);
			*to->l << fnParam(ptr, ptrRel(ptrA, Offset()));
			*to->l << fnCallRef(cloneFn(result.type)->ref(), false, ptr, res);
		}

	}

	void Function::asyncAutoCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result) {
		asyncAutoCall(to, params, result, null);
	}

	void Function::asyncAutoCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, CodeResult *id) {
		RunOn r = runOn();
		if (to->runOn.canRun(r)) {
			asyncLocalCall(to, params, result, id);
		} else {
			asyncThreadCall(to, params, result, id);
		}
	}

	void Function::asyncLocalCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result) {
		asyncLocalCall(to, params, result, null);
	}

	void Function::asyncLocalCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, CodeResult *id) {
		asyncCall(to, params, code::Operand(), result, id, false);
	}

	void Function::asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result) {
		asyncThreadCall(to, params, result, code::Operand(), null);
	}

	void Function::asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Operand thread) {
		asyncThreadCall(to, params, result, thread, null);
	}

	void Function::asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, CodeResult *id) {
		asyncThreadCall(to, params, result, code::Operand(), id);
	}

	void Function::asyncThreadCall(CodeGen *to, Array<code::Operand> *params, CodeResult *result, code::Operand thread, CodeResult *id) {
		asyncCall(to, params, thread, result, id, true);
	}

	void Function::asyncCall(CodeGen *to, Array<code::Operand> *params, code::Operand thread,
							CodeResult *result, CodeResult *id, bool copy) {
		using namespace code;

		// Figure out what thread to pass to the spawn function.
		if (thread.empty())
			thread = ptrConst(0);
		if (runOn().state != RunOn::any)
			thread = findThread(to, params);

		Engine &e = engine();
		CodeGen *sub = to->child();
		*to->l << begin(sub->block);

		// Spill any registers to memory if necessary...
		params = spillRegisters(sub, params);

		// Create the parameters.
		Var par = createFnCall(sub, this->params, params, copy);

		// Create the result object.
		Type *futureT = wrapFuture(e, this->result).type;
		code::Var resultPos = result->safeLocation(sub, thisPtr(futureT));
		allocObject(sub, futureT->defaultCtor(), new (this) Array<Operand>(), resultPos);
		result->created(sub);

		// If we don't need copies, tell the future that as well.
		if (!copy) {
			*to->l << fnParam(e.ptrDesc(), resultPos);
			*to->l << fnCall(e.ref(builtin::futureNoClone), true);
		}

		// Get the thunk.
		Ref thunk = Ref(threadThunk());

		TypeDesc *ptr = e.ptrDesc();

		// Spawn the thread!
		*to->l << lea(ptrA, par);
		*to->l << fnParam(ptr, ref()); // fn
		*to->l << fnParam(byteDesc(e), toOp(isMember())); // member
		*to->l << fnParam(ptr, thunk); // thunk
		*to->l << fnParam(ptr, ptrA); // params
		*to->l << fnParam(ptr, resultPos); // result
		*to->l << fnParam(ptr, thread); // on

		// Take care of "id" if we need it.
		if (id) {
			*to->l << fnCall(e.ref(builtin::spawnId), false, longDesc(e), rax);
			if (id->needed())
				*to->l << mov(id->location(to), rax);
		} else {
			*to->l << fnCall(e.ref(builtin::spawnFuture), false);
		}

		// Now, we're done!
		*to->l << end(sub->block);
	}

	code::RefSource *Function::threadThunk() {
		using namespace code;

		if (threadThunkRef)
			return threadThunkRef;

		Binary *threadThunkCode = storm::callThunk(result, params);
		threadThunkRef = new (this) NamedSource(this, Char('t'));
		threadThunkRef->set(threadThunkCode);
		return threadThunkRef;
	}

	code::Ref Function::thunkRef() {
		// This is the same thing as the thread thunk nowadays.
		return code::Ref(threadThunk());
	}


	/**
	 * Low-level functions called by the generated code.
	 */

	void spawnThreadResult(const void *fn, bool member, os::CallThunk thunk, void **params, void *result, Thread *on) {
		os::FnCallRaw call(params, thunk);
		os::FutureSema<os::Sema> future;
		const os::Thread *thread = on ? &on->thread() : null;
		os::UThread::spawnRaw(fn, member, null, call, future, result, thread);
		future.result();
	}

	void spawnThreadFuture(const void *fn, bool member, os::CallThunk thunk, void **params, FutureBase *result, Thread *on) {
		os::FnCallRaw call(params, thunk);
		const os::Thread *thread = on ? &on->thread() : null;
		os::UThread::spawnRaw(fn, member, null, call, *result->rawFuture(), result->rawResult(), thread);
	}

	// As 'spawnThreadFuture', but returns a thread ID (same as currentUThread() returns in Storm) and detaches the future.
	Word spawnThreadId(const void *fn, bool member, os::CallThunk thunk, void **params, FutureBase *result, Thread *on) {
		os::FnCallRaw call(params, thunk);
		const os::Thread *thread = on ? &on->thread() : null;
		return os::UThread::spawnRaw(fn, member, null, call, *result->rawFuture(), result->rawResult(), thread).id();
	}

	/**
	 * CppFunction.
	 */

	CppMemberFunction::CppMemberFunction(Value result, Str *name, Array<Value> *params, const void *original) :
		Function(result, name, params), original(original) {}

	const void *CppMemberFunction::originalPtr() {
		return original;
	}


	/**
	 * Convenience functions.
	 */

	Function *inlinedFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, Fn<void, InlineParams> *fn) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) InlineCode(fn));
		return r;
	}

	Function *nativeFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, const void *fn) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) StaticCode(fn));
		return r;
	}

	Function *nativeEngineFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, const void *fn) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) StaticEngineCode(fn));
		return r;
	}

	Function *lazyFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, Fn<CodeGen *> *generate) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) LazyCode(generate));
		return r;
	}

	Function *dynamicFunction(Engine &e, Value result, const wchar *name, Array<Value> *params, code::Listing *src) {
		Function *r = new (e) Function(result, new (e) Str(name), params);
		r->setCode(new (e) DynamicCode(src));
		return r;
	}

}
