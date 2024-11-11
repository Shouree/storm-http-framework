#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"
#include "Engine.h"
#include "Core/CloneEnv.h"
#include "Lib/Clone.h"

namespace storm {

	CodeGen::CodeGen(RunOn thread, Bool member, Value result) : runOn(thread), res(result) {
		l = new (this) code::Listing(member, result.desc(engine()));
		block = l->root();
	}

	CodeGen::CodeGen(Function *f) : runOn(f->runOn()), res(f->result) {
		l = new (this) code::Listing(f->isMember(), res.desc(engine()));
		block = l->root();
	}

	CodeGen::CodeGen(RunOn thread, code::Listing *to) : runOn(thread), l(to), block(to->root()) {}

	CodeGen::CodeGen(RunOn thread, code::Listing *to, code::Block block) : runOn(thread), l(to), block(block) {}

	CodeGen::CodeGen(CodeGen *me, code::Block b) : runOn(me->runOn), l(me->l), block(b), res(me->res) {}

	CodeGen *CodeGen::child(code::Block b) {
		return new (this) CodeGen(this, b);
	}

	CodeGen *CodeGen::child() {
		code::Block b = l->createBlock(block);
		return child(b);
	}

	void CodeGen::deepCopy(CloneEnv *env) {
		clone(l, env);
	}

	code::Var CodeGen::createParam(Value type) {
		return l->createParam(type.desc(engine()));
	}

	VarInfo CodeGen::createVar(Value type) {
		return createVar(type, block);
	}

	VarInfo CodeGen::createVar(Value type, code::Block in) {
		code::FreeOpt opt = code::freeOnBoth;
		code::Operand dtor;
		bool needsActivation = !type.isAsmType() && !type.ref;

		if (needsActivation) {
			dtor = type.destructor();
			opt = opt | code::freePtr | code::freeInactive;
		}

		return VarInfo(l->createVar(in, type.size(), dtor, opt), needsActivation);
	}

	Value CodeGen::result() const {
		return res;
	}

	void CodeGen::returnValue(code::Var value) {
		using namespace code;
		if (value == Var())
			*l << fnRet();
		else
			*l << fnRet(value);
	}

	void CodeGen::toS(StrBuf *to) const {
		*to << L"Running on: " << runOn << L"\n";
		*to << L"Current block: " << block << L"\n";
		*to << l;
	}


	/**
	 * VarInfo.
	 */

	VarInfo::VarInfo() : v(), needsActivation(false) {}

	VarInfo::VarInfo(code::Var v) : v(v), needsActivation(false) {}

	VarInfo::VarInfo(code::Var v, Bool needsActivation) : v(v), needsActivation(needsActivation) {}

	void VarInfo::created(CodeGen *gen) {
		using namespace code;

		if (!needsActivation)
			return;

		*gen->l << activate(v);
	}


	/**
	 * CodeResult.
	 */

	CodeResult::CodeResult() {}

	CodeResult::CodeResult(Value type, code::Block block) : block(block), t(type) {}

	CodeResult::CodeResult(Value type, code::Var var) : variable(var), t(type) {}

	CodeResult::CodeResult(Value type, VarInfo var) : variable(var), t(type) {}

	code::Var CodeResult::location(CodeGen *s) {
		assert(needed(), L"Trying to get the location of an unneeded result. Use 'safeLocation' instead.");

		if (variable.v == code::Var()) {
			if (block == code::Block()) {
				variable = s->createVar(t);
			} else {
				variable = s->createVar(t, block);
			}
		}

		return variable.v;
	}

	code::Var CodeResult::safeLocation(CodeGen *s, Value type) {
		if (needed()) {
			return location(s);
		} else if (variable.v == code::Var()) {
			variable = s->createVar(type);
			return variable.v;
		} else {
			return variable.v;
		}
	}

	void CodeResult::created(CodeGen *s) {
		if (variable.v != code::Var())
			variable.created(s);
	}

	Bool CodeResult::suggest(CodeGen *s, code::Var v) {
		code::Listing *l = s->l;

		if (variable.v != code::Var())
			return false;

		// TODO: Cases that hit here could maybe be optimized somehow. This is common with the
		// return value, which will almost always have to get its lifetime extended a bit. Maybe
		// implement the possibility to move variables to a more outer scope?
		if (block != code::Block() && !l->accessible(v, block))
			return false;

		variable = VarInfo(v);
		return true;
	}

	Bool CodeResult::suggest(CodeGen *s, code::Operand v) {
		if (v.type() == code::opVariable)
			return suggest(s, v.var());
		return false;
	}

	Value CodeResult::type() const {
		return t;
	}

	Bool CodeResult::needed() const {
		return t != Value();
	}

	CodeResult *CodeResult::split(CodeGen *s) {
		if (!needed()) {
			return new (this) CodeResult();
		}

		if (variable.v == code::Var()) {
			// Create the variable now.
			location(s);
		}

		// Give it a variable that does not need to be created.
		return new (this) CodeResult(t, VarInfo(variable.v, false));
	}


	code::Var createFnCallParams(CodeGen *to, Nat paramCount) {
		using namespace code;

		Size s = Size::sPtr * paramCount;
		return to->l->createVar(to->block, s);
	}

	void setFnParam(CodeGen *to, code::Var params, Nat paramId, code::Operand param) {
		using namespace code;

		Offset o = Offset::sPtr * paramId;

		*to->l << lea(ptrA, param);
		*to->l << mov(ptrRel(params, o), ptrA);
	}

	static bool needsCloneEnv(Value type) {
		if (type.ref || type.isPrimitive())
			return false;
		else if (type.isActor())
			return false;
		else
			return true;
	}

	static code::Operand cloneParam(CodeGen *to, Value formal, code::Operand actual, code::Var env) {
		using namespace code;

		if (!needsCloneEnv(formal))
			return actual;

		Engine &e = to->engine();
		if (formal.isObject()) {
			VarInfo clone = to->createVar(formal);

			*to->l << fnParam(formal.desc(e), actual);
			*to->l << fnParam(e.ptrDesc(), env);
			*to->l << fnCall(cloneFnEnv(formal.type)->ref(), false, e.ptrDesc(), clone.v);
			clone.created(to);

			return clone.v;
		} else {
			Function *toCall = formal.type->deepCopyFn();
			if (!toCall)
				// No 'deepCopy'? No problem!
				return actual;

			VarInfo clone = to->createVar(formal);

			*to->l << lea(ptrC, clone.v);
			*to->l << lea(ptrA, actual);
			*to->l << fnParam(e.ptrDesc(), ptrC);
			*to->l << fnParam(e.ptrDesc(), ptrA);
			*to->l << fnCall(formal.copyCtor(), true);
			clone.created(to);

			*to->l << lea(ptrC, clone.v);
			*to->l << fnParam(e.ptrDesc(), ptrC);
			*to->l << fnParam(e.ptrDesc(), env);
			*to->l << fnCall(toCall->ref(), true);
			return clone.v;
		}
	}

	code::Var createFnCall(CodeGen *to, Array<Value> *formals, Array<code::Operand> *actuals, Bool copy) {
		using namespace code;
		assert(formals->count() == actuals->count(), L"Size of formals array does not match actuals!");

		// Examine if we need to create a CloneEnv.
		bool needEnv = false;
		if (copy)
			for (Nat i = 0; i < formals->count(); i++)
				needEnv |= needsCloneEnv(formals->at(i));

		// Create it if required.
		Var env;
		if (needEnv)
			env = allocObject(to, CloneEnv::stormType(to->engine()));

		// Create the FnCall array.
		Var call = createFnCallParams(to, formals->count());

		// Start copying the parameters...
		for (Nat i = 0; i < formals->count(); i++) {
			Operand clone = actuals->at(i);
			if (copy)
				clone = cloneParam(to, formals->at(i), clone, env);
			setFnParam(to, call, i, clone);
		}

		return call;
	}

	void allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to) {
		using namespace code;

		Type *type = ctor->params->at(0).type;
		assert(Value(type).isObject(), L"Must allocate a class type! (not " + ::toS(type) + L")");

		Engine &e = ctor->engine();

		*s->l << fnParam(e.ptrDesc(), type->typeRef());
		*s->l << fnCall(e.ref(builtin::alloc), false, e.ptrDesc(), to);

		CodeResult *r = new (s) CodeResult();
		params = new (s) Array<code::Operand>(*params);
		params->insert(0, to);
		ctor->autoCall(s, params, r);
	}

	code::Var allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params) {
		code::Var r = s->l->createVar(s->block, Size::sPtr);
		allocObject(s, ctor, params, r);
		return r;
	}

	code::Var allocObject(CodeGen *s, Type *type) {
		Function *ctor = type->defaultCtor();
		if (!ctor) {
			Str *msg = TO_S(s, S("Can not allocate ") << type->identifier() << S(" using the default constructor."));
			throw new (s) InternalError(msg);
		}
		return allocObject(s, ctor, new (type->engine) Array<code::Operand>());
	}

	static void addThunkParam(code::Listing *l, Value param, Nat id) {
		using namespace code;

		Offset o = Offset::sPtr * id;
		*l << fnParamRef(param.desc(l->engine()), ptrRel(ptrB, o));
	}

	code::Binary *callThunk(Value result, Array<Value> *formals) {
		using namespace code;
		Engine &e = formals->engine();

		CodeGen *s = new (e) CodeGen(RunOn(), false, Value());
		Listing *l = s->l;
		TypeDesc *rDesc = result.desc(e);

		// Parameters:
		Var fn = l->createParam(e.ptrDesc());
		Var member = l->createParam(byteDesc(e));
		Var params = l->createParam(e.ptrDesc());
		Var first = l->createParam(e.ptrDesc());
		Var output = l->createParam(e.ptrDesc());

		*l << prolog();
		*l << mov(ptrB, params);
		// Do the function call. We have four alternatives:
		// 1: Neither a first param, nor member (lPlain).
		// 2: We have a first param, but not a member (lFirst).
		// 3: No first param, but a member (lMember).
		// 4: Both a first param and a member (lBoth).
		Label lNoMember = l->label();
		Label lPlain = l->label();
		Label lFirst = l->label();
		Label lMember = l->label();
		Label lBoth = l->label();

		if (!result.returnInReg()) {
			// These cases (lBoth and lMember) can be ignored if we are returning something in a register.
			*l << cmp(member, byteConst(0));
			*l << jmp(lNoMember, ifEqual);
			*l << cmp(first, ptrConst(0));
			*l << jmp(lMember, ifEqual);

			*l << lBoth;
			*l << fnParam(e.ptrDesc(), first);
			for (Nat i = 0; i < formals->count(); i++)
				addThunkParam(l, formals->at(i), i);
			*l << fnCallRef(fn, true, rDesc, output);
			*l << fnRet();

			*l << lMember;
			for (Nat i = 0; i < formals->count(); i++)
				addThunkParam(l, formals->at(i), i);
			*l << fnCallRef(fn, true, rDesc, output);
			*l << fnRet();
		}

		*l << lNoMember;
		*l << cmp(first, ptrConst(0));
		*l << jmp(lPlain, ifEqual);

		*l << lFirst;
		*l << fnParam(e.ptrDesc(), first);
		for (Nat i = 0; i < formals->count(); i++)
			addThunkParam(l, formals->at(i), i);
		*l << fnCallRef(fn, false, rDesc, output);
		*l << fnRet();

		*l << lPlain;
		for (Nat i = 0; i < formals->count(); i++)
			addThunkParam(l, formals->at(i), i);
		*l << fnCallRef(fn, false, rDesc, output);
		*l << fnRet();

		return new (e) Binary(e.arena(), l);
	}


	Array<code::Operand> *spillRegisters(CodeGen *s, Array<code::Operand> *params) {
		Bool hasReg = false;
		for (Nat i = 0; i < params->count(); i++) {
			hasReg |= params->at(i).hasRegister();
		}

		if (!hasReg)
			return params;

		Array<code::Operand> *result = new (params) Array<code::Operand>(*params);

		for (Nat i = 0; i < result->count(); i++) {
			code::Operand &op = result->at(i);
			if (!op.hasRegister())
				continue;

			// Create a variable for this type.
			code::Var v = s->l->createVar(s->block, op.size());
			*s->l << mov(v, op);
			op = code::Operand(v);
		}

		return result;
	}


	Function *findStormFn(NameSet *inside, const wchar *name, Array<Value> *params) {
		SimplePart *part = new (inside) SimplePart(new (inside) Str(name), params);
		Function *result = as<Function>(inside->find(part, Scope()));
		if (result)
			return result;

		Str *msg = TO_S(inside, S("The function ") << part << S(" is not inside ") << inside->identifier() << S("."));
		throw new (inside) InternalError(msg);
	}

	Function *findStormFn(Value inside, const wchar *name, Array<Value> *params) {
		return findStormFn(inside.type, name, params);
	}

	Function *findStormFn(Value inside, const wchar *name) {
		return findStormFn(inside, name, valList(inside.type->engine, 0));
	}

	Function *findStormFn(Value inside, const wchar *name, Value param0) {
		return findStormFn(inside, name, valList(inside.type->engine, 1, param0));
	}

	Function *findStormFn(Value inside, const wchar *name, Value param0, Value param1) {
		return findStormFn(inside, name, valList(inside.type->engine, 2, param0, param1));
	}

	Function *findStormFn(Value inside, const wchar *name, Value param0, Value param1, Value param2) {
		return findStormFn(inside, name, valList(inside.type->engine, 3, param0, param1, param2));
	}

	Function *findStormMemberFn(Type *inside, const wchar *name, Array<Value> *params) {
		return findStormMemberFn(thisPtr(inside), name, params);
	}

	Function *findStormMemberFn(Value inside, const wchar *name, Array<Value> *params) {
		Array<Value> *copy = new (inside.type) Array<Value>(*params);
		copy->insert(0, inside);
		return findStormFn(inside.type, name, copy);
	}

	Function *findStormMemberFn(Value inside, const wchar *name) {
		return findStormMemberFn(inside, name, valList(inside.type->engine, 0));
	}

	Function *findStormMemberFn(Value inside, const wchar *name, Value param0) {
		return findStormMemberFn(inside, name, valList(inside.type->engine, 1, param0));
	}

	Function *findStormMemberFn(Value inside, const wchar *name, Value param0, Value param1) {
		return findStormMemberFn(inside, name, valList(inside.type->engine, 2, param0, param1));
	}

	Function *findStormMemberFn(Value inside, const wchar *name, Value param0, Value param1, Value param2) {
		return findStormMemberFn(inside, name, valList(inside.type->engine, 3, param0, param1, param2));
	}

}
