#include "stdafx.h"
#include "Fn.h"
#include "TemplateList.h"
#include "Engine.h"
#include "Exception.h"
#include "Clone.h"
#include "Core/Fn.h"
#include "Core/Str.h"

namespace storm {

	Type *createFn(Str *name, ValueArray *params) {
		return new (name) FnType(name, params);
	}

	Type *fnType(Array<Value> *params) {
		Engine &e = params->engine();
		TemplateList *l = e.cppTemplate(ArrayId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'fnType'.");
		Type *found = as<Type>(to->find(S("Fn"), params, Scope()));
		if (!found)
			throw new (params) InternalError(S("Can not find the function type!"));
		return found;
	}

	static void fnCopy(void *mem, FnBase *src) {
		new (Place(mem)) FnBase(*src);
	}

	FnType::FnType(Str *name, ValueArray *params) : Type(name, params->toArray(), typeClass) {
		setSuper(FnBase::stormType(engine));
	}

	Value FnType::result() {
		return params->at(0);
	}

	Array<Value> *FnType::parameters() {
		Array<Value> *c = new (this) Array<Value>();
		c->reserve(params->count() - 1);
		for (Nat i = 1; i < params->count(); i++)
			c->push(params->at(i));
		return c;
	}

	Bool FnType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, t), address(&fnCopy)));

		Array<Value> *fnCall = clone(params);
		fnCall->at(0) = t;
		add(lazyFunction(e, params->at(0), S("call"), fnCall, fnPtr(e, &FnType::callCode, this)));

		Value rawFn(RawFn::stormType(e));
		add(lazyFunction(e, rawFn, S("rawCall"), valList(e, 1, t), fnPtr(e, &FnType::rawCallCode, this)));

		return Type::loadAll();
	}

	CodeGen *FnType::callCode() {
		using namespace code;
		Engine &e = engine;

		Value result = params->at(0);

		TypeDesc *ptr = e.ptrDesc();
		CodeGen *s = new (e) CodeGen(RunOn(), true, result);
		Var me = s->l->createParam(ptr);

		// Create parameters.
		Array<Value> *formal = new (e) Array<Value>();
		Array<Operand> *input = new (e) Array<Operand>();
		for (nat i = 1; i < params->count(); i++) {
			formal->push(params->at(i));
			input->push(s->createParam(params->at(i)));
		}

		// Create the thunk if not done already.
		if (!thunk) {
			thunk = new (this) NamedSource(this, Char('t'));
			thunk->set(callThunk(result, formal));
		}

		// Figure out if the first parameter is a TObject and pass it on in case the FnPtr needs to know.
		Operand firstTObj = ptrConst(Offset());
		if (params->count() > 1 && params->at(1).isActor())
			firstTObj = input->at(0);

		*s->l << prolog();

		// Do we need to clone the parameters and the result?
		Var needClone = s->l->createVar(s->l->root(), Size::sByte);
		*s->l << fnParam(ptr, me);
		*s->l << fnParam(ptr, firstTObj);
		*s->l << fnCall(e.ref(builtin::fnNeedsCopy), true, byteDesc(e), needClone);

		// Handle parameters.
		Label doCopy = s->l->label();
		Label done = s->l->label();
		*s->l << cmp(needClone, byteConst(0));
		*s->l << jmp(doCopy, ifNotEqual);
		Var fnParamsPlain = createFnCall(s, formal, input, false);
		*s->l << lea(ptrC, fnParamsPlain);
		*s->l << jmp(done);
		*s->l << doCopy;
		Var fnParamsClone = createFnCall(s, formal, input, true);
		*s->l << lea(ptrC, fnParamsClone);
		*s->l << done;

		// Call the function!
		if (!result.returnInReg()) {
			// Create temporary storage for the result.
			Var resultMem = s->l->createVar(s->l->root(), result.desc(e));
			*s->l << lea(ptrB, resultMem);

			// Value -> call deepCopy if present.
			*s->l << fnParam(ptr, me); // b
			*s->l << fnParam(ptr, ptrB); // output
			*s->l << fnParam(ptr, Ref(thunk)); // thunk
			*s->l << fnParam(ptr, ptrC); // params
			*s->l << fnParam(ptr, firstTObj); // first
			*s->l << fnCall(e.ref(builtin::fnCall), false);

			// Call 'deepCopy'
			if (Function *call = result.type->deepCopyFn()) {
				Var env = allocObject(s, CloneEnv::stormType(e));
				*s->l << lea(ptrB, resultMem);
				*s->l << fnParam(ptr, ptrB);
				*s->l << fnParam(ptr, env);
				*s->l << fnCall(call->ref(), true);
			}

			*s->l << fnRet(resultMem);
		} else if (result.type == null || result.isPrimitive() || result.isActor()) {
			// No need to copy!
			Var r;
			if (result != Value()) {
				r = s->l->createVar(s->l->root(), result.size());
				*s->l << lea(ptrA, r);
			} else {
				*s->l << mov(ptrA, ptrConst(Offset()));
			}
			*s->l << fnParam(ptr, me); // b
			*s->l << fnParam(ptr, ptrA); // output
			*s->l << fnParam(ptr, Ref(thunk)); // thunk
			*s->l << fnParam(ptr, ptrC); // params
			*s->l << fnParam(ptr, firstTObj); // first
			*s->l << fnCall(e.ref(builtin::fnCall), false);

			if (result != Value())
				*s->l << fnRet(r);
			else
				*s->l << fnRet();
		} else {
			// Class! Call 'core.clone'.
			Var r = s->l->createVar(s->l->root(), result.size());
			*s->l << lea(ptrA, r);
			*s->l << fnParam(ptr, me); // b
			*s->l << fnParam(ptr, ptrA); // output
			*s->l << fnParam(ptr, Ref(thunk)); // thunk
			*s->l << fnParam(ptr, ptrC); // params
			*s->l << fnParam(ptr, firstTObj); // first
			*s->l << fnCall(e.ref(builtin::fnCall), false);

			if (Function *call = cloneFn(result.type)) {
				*s->l << fnParam(ptr, r);
				*s->l << fnCall(call->ref(), false, ptr, ptrA);
			} else {
				*s->l << mov(ptrA, r);
			}

			*s->l << fnRet(ptrA);
		}

		return s;
	}

	CodeGen *FnType::rawCallCode() {
		using namespace code;
		Engine &e = engine;

		if (!rawCall)
			rawCall = createRawCall();

		Value result(RawFn::stormType(e));

		TypeDesc *ptr = e.ptrDesc();
		CodeGen *s = new (e) CodeGen(RunOn(), true, result);
		s->l->createParam(ptr);

		Var v = s->l->createVar(s->l->root(), Size::sPtr);

		*s->l << prolog();
		*s->l << mov(v, Ref(rawCall));
		*s->l << fnRet(v);

		return s;
	}

	code::RefSource *FnType::createRawCall() {
		using namespace code;
		Engine &e = engine;

		CodeGen *s = new (e) CodeGen(RunOn(), false, Value());
		Listing *l = s->l;

		// Parameters:
		Var fnBase = l->createParam(e.ptrDesc());
		Var out = l->createParam(e.ptrDesc());
		Var paramArray = l->createParam(e.ptrDesc());

		*l << prolog();

		Array<Value> *callParams = clone(params);
		callParams->at(0) = thisPtr(this);
		Function *callFn = as<Function>(find(S("call"), callParams, Scope()));
		assert(callFn, L"Could not find 'call' inside a Fn object.");

		*l << mov(ptrA, paramArray);

		// Add parameters one by one.
		*l << fnParam(callParams->at(0).desc(e), fnBase);
		for (Nat i = 1; i < params->count(); i++)
			*l << fnParamRef(params->at(i).desc(e), ptrRel(ptrA, Offset::sPtr * (i - 1)));

		*l << fnCallRef(callFn->ref(), true, params->at(0).desc(e), out);

		*l << fnRet();

		RefSource *result = new (e) NamedSource(this, Char('r'));
		result->set(new (e) Binary(e.arena(), l));
		return result;
	}

	void CODECALL fnCallRaw(FnBase *b, void *output, os::CallThunk thunk, void **params, TObject *first) {
		// TODO: We can provide a single CloneEnv so that all parameters are cloned uniformly.
		os::FnCallRaw call(params, thunk);
		b->callRawI(output, call, first, null);
	}

	class RefFnTarget : public FnTarget {
	public:
		RefFnTarget(code::Ref ref) : ref(ref) {}

		// What we refer to.
		code::Ref ref;

		// Clone.
		virtual void cloneTo(void *to, size_t size) const {
			assert(size >= sizeof(*this));
			new (to) RefFnTarget(ref);
		}

		// Get the pointer.
		virtual const void *ptr() const {
			return ref.address();
		}


		// Get label.
		virtual void toS(StrBuf *to) const {
			*to << ref.title();
		}
	};

	static FnBase *pointerI(Function *target, RootObject *thisPtr) {
		Array<Value> *p = clone(target->params);
		if (!thisPtr) {
			p->insert(0, target->result);
		} else if (p->count() > 0 && p->at(0).canStore(Value(runtime::typeOf(thisPtr)))) {
			p->at(0) = target->result;
		} else {
			Str *msg;
			if (p->count() == 0)
				msg = TO_S(target, S("Attempted to bind a non-existent first parameter of a function pointer ")
						S("when calling 'pointer'."));
			else
				msg = TO_S(target, S("Type mismatch for object passed to 'pointer'. Expected ")
						<< p->at(0) << S(", but got ") << Value(runtime::typeOf(thisPtr)));

			throw new (target) UsageError(msg);
		}
		Type *t = fnType(p);

		Thread *thread = null;
		RunOn runOn = target->runOn();
		if (runOn.state == RunOn::named)
			thread = runOn.thread->thread();

		void *mem = runtime::allocObject(sizeof(FnBase), t);
		FnBase *c = new (Place(mem)) FnBase(RefFnTarget(target->ref()),
											thisPtr,
											target->isMember(),
											thread);
		runtime::setVTable(c);
		return c;
	}

	FnBase *pointer(Function *target) {
		return pointerI(target, null);
	}

	FnBase *pointer(Function *target, Object *thisPtr) {
		return pointerI(target, thisPtr);
	}

	FnBase *pointer(Function *target, TObject *thisPtr) {
		return pointerI(target, thisPtr);
	}

	FnBase *fnCreateRaw(Type *type, code::RefSource *to, Thread *thread, RootObject *thisPtr, Bool memberFn) {
		void *mem = runtime::allocObject(sizeof(FnBase), type);
		FnBase *c = new (Place(mem)) FnBase(RefFnTarget(to), thisPtr, memberFn, thread);
		runtime::setVTable(c);
		return c;
	}

}
