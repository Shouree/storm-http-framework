#include "stdafx.h"
#include "TypeCtor.h"
#include "Type.h"
#include "Exception.h"
#include "Package.h"
#include "Engine.h"
#include "Core/Str.h"

namespace storm {
	using namespace code;

	TypeDefaultCtor::TypeDefaultCtor(Type *owner)
		: Function(Value(), new (owner) Str(Type::CTOR), new (owner) Array<Value>(1, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeDefaultCtor::generate, this)));
	}

	Bool TypeDefaultCtor::pure() const {
		// Not a value: not pure!
		if ((owner->typeFlags() & typeValue) != typeValue)
			return false;

		Array<MemberVar *> *v = owner->variables();
		for (Nat i = 0; i < v->count(); i++) {
			MemberVar *var = v->at(i);
			Value t = var->type;

			if (!t.type)
				continue;

			// Primitives are trivial to copy.
			if (t.isPrimitive())
				continue;

			// Classes and actors need initialization!
			if (t.isObject())
				return false;

			Function *ctor = t.type->defaultCtor();
			if (!ctor)
				continue;

			if (!ctor->pure())
				return false;
		}

		return true;
	}

	CodeGen *TypeDefaultCtor::generate() {
		CodeGen *t = new (this) CodeGen(runOn(), true, thisPtr(owner));
		Listing *l = t->l;

		TypeDesc *ptr = engine().ptrDesc();
		Var me = l->createParam(ptr);

		*l << prolog();

		Type *super = owner->super();
		if (super == TObject::stormType(engine())) {
			// Run the TObject constructor if possible.
			NamedThread *thread = runOn().thread;
			if (!thread) {
				Str *msg = TO_S(this, S("Can not use TypeDefaultCtor with TObjects without specifying a thread. ")
								S("Set a thread for ") << owner->identifier() << S(" and try again."));
				throw new (this) InternalError(msg);
			}

			Array<Value> *params = new (this) Array<Value>();
			params->push(thisPtr(super));
			params->push(thisPtr(Thread::stormType(engine())));
			Function *ctor = as<Function>(super->find(Type::CTOR, params, Scope()));
			if (!ctor)
				throw new (this) InternalError(S("Can not find the default constructor for TObject!"));

			*l << fnParam(ptr, me);
			*l << fnParam(ptr, thread->ref());
			*l << fnCall(ctor->directRef(), true);
		} else if (super) {
			// Find and run the parent constructor.
			Function *ctor = super->defaultCtor();
			if (!ctor) {
				Str *msg = TO_S(this, S("Can not use TypeDefaultCtor if no default constructor ")
								S("for the parent type is present. See parent class for ")
								<< owner->identifier());
				throw new (this) InternalError(msg);
			}

			*l << fnParam(ptr, me);
			*l << fnCall(ctor->directRef(), true);
		} else {
			// No parent constructor to run.
		}

		// Initialize all variables...
		Array<MemberVar *> *vars = owner->variables();
		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);
			if (Function *init = v->initializer) {
				// Use initializer.
				Var tmp = l->createVar(t->block, Size::sPtr);

				*l << mov(ptrA, me);
				*l << add(ptrA, ptrConst(v->offset()));
				*l << mov(tmp, ptrA);

				init->autoCallRef(t, new (this) Array<code::Operand>(), tmp);
			} else if (v->type.isPrimitive()) {
				// No need for initialization.
			} else if (v->type.isValue()) {
				Function *ctor = v->type.type->defaultCtor();
				if (!ctor) {
					Str *msg = TO_S(this, S("Can not use TypeDefaultCtor if a member does not have a default ")
									S("constructor. See ") << owner->identifier());
					throw new (this) InternalError(msg);
				}
				*l << mov(ptrA, me);
				*l << add(ptrA, ptrConst(v->offset()));
				*l << fnParam(ptr, ptrA);
				*l << fnCall(ctor->ref(), true);
			} else {
				Function *ctor = v->type.type->defaultCtor();
				if (!ctor) {
					Str *msg = TO_S(this, S("Can not use TypeDefaultCtor if a member does not have a default ")
									S("constructor. See ") << owner->identifier());
					throw new (this) InternalError(msg);
				}

				Var created = allocObject(t, ctor, new (this) Array<Operand>());
				*l << mov(ptrA, me);
				*l << mov(ptrRel(ptrA, v->offset()), created);
			}
		}

		if (owner->typeFlags() & typeClass) {
			// Set the VTable.
			owner->vtable()->insert(l, me);
		}

		*l << fnRet(me);

		return t;
	}


	TypeCopyCtor::TypeCopyCtor(Type *owner)
		: Function(Value(), new (owner) Str(Type::CTOR), new (owner) Array<Value>(2, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeCopyCtor::generate, this)));
	}

	Bool TypeCopyCtor::pure() const {
		// Not a value: not pure!
		if ((owner->typeFlags() & typeValue) != typeValue)
			return false;

		Array<MemberVar *> *v = owner->variables();
		for (Nat i = 0; i < v->count(); i++) {
			MemberVar *var = v->at(i);
			Value t = var->type;

			if (!t.type)
				continue;

			// Classes, actors and built-ins are trivial to copy!
			if (t.isAsmType())
				continue;

			Function *ctor = t.type->copyCtor();
			if (!ctor)
				continue;

			if (!ctor->pure())
				return false;
		}

		return true;
	}

	CodeGen *TypeCopyCtor::generate() {
		CodeGen *t = new (this) CodeGen(runOn(), true, thisPtr(owner));
		Listing *l = t->l;

		TypeDesc *ptr = engine().ptrDesc();
		Var me = l->createParam(ptr);
		Var src = l->createParam(ptr);

		*l << prolog();

		if (Type *super = owner->super()) {
			Function *ctor = super->copyCtor();
			if (!ctor) {
				Str *msg = TO_S(this, S("No copy constructor for ") << super->identifier()
								<< S(", required from ") << owner->identifier());
				throw new (this) InternalError(msg);
			}

			*l << fnParam(ptr, me);
			*l << fnParam(ptr, src);
			*l << fnCall(ctor->ref(), true);
		}

		// Copy all variables.
		Array<MemberVar *> *vars = owner->variables();
		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);

			*l << mov(ptrA, me);
			*l << mov(ptrC, src);
			if (!v->type.isAsmType()) {
				Function *ctor = v->type.type->copyCtor();
				if (!ctor) {
					Str *msg = TO_S(this, S("No copy constructor for ") << v->type.type->identifier()
									<< S(", required from ") << owner->identifier());
					throw new (this) InternalError(msg);
				}

				*l << add(ptrA, ptrConst(v->offset()));
				*l << add(ptrC, ptrConst(v->offset()));
				*l << fnParam(ptr, ptrA);
				*l << fnParam(ptr, ptrC);
				*l << fnCall(ctor->ref(), true);
			} else {
				// Pointer or built-in.
				*l << mov(xRel(v->type.size(), ptrA, v->offset()), xRel(v->type.size(), ptrC, v->offset()));
			}
		}

		if (owner->typeFlags() & typeClass) {
			// Set the VTable.
			owner->vtable()->insert(l, me);
		}

		*l << fnRet(me);

		return t;
	}


	TypeAssign::TypeAssign(Type *owner)
		: Function(thisPtr(owner), new (owner) Str(L"="), new (owner) Array<Value>(2, thisPtr(owner))),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeAssign::generate, this)));
	}

	Bool TypeAssign::pure() const {
		// Not a value: not pure!
		if ((owner->typeFlags() & typeValue) != typeValue)
			return false;

		Array<MemberVar *> *v = owner->variables();
		for (Nat i = 0; i < v->count(); i++) {
			MemberVar *var = v->at(i);
			Value t = var->type;

			if (!t.type)
				continue;

			// Classes and actors are trivial to copy.
			if (t.isAsmType())
				continue;

			Function *assign = t.type->assignFn();
			if (!assign)
				continue;

			if (!assign->pure())
				return false;
		}

		return true;
	}

	CodeGen *TypeAssign::generate() {
		CodeGen *t = new (this) CodeGen(runOn(), true, thisPtr(owner));
		Listing *l = t->l;

		TypeDesc *ptr = engine().ptrDesc();
		Var me = l->createParam(ptr);
		Var src = l->createParam(ptr);

		*l << prolog();

		if (Type *super = owner->super()) {
			Function *ctor = super->copyCtor();
			if (!ctor) {
				Str *msg = TO_S(this, S("No assignment operator for ") << super->identifier()
								<< S(", required from ") << owner->identifier());
				throw new (this) InternalError(msg);
			}

			*l << fnParam(ptr, me);
			*l << fnParam(ptr, src);
			*l << fnCall(ctor->ref(), true);
		}

		// Copy all variables.
		Array<MemberVar *> *vars = owner->variables();
		for (nat i = 0; i < vars->count(); i++) {
			MemberVar *v = vars->at(i);

			*l << mov(ptrA, me);
			*l << mov(ptrC, src);
			if (!v->type.isAsmType()) {
				Function *ctor = v->type.type->assignFn();
				if (!ctor) {
					Str *msg = TO_S(this, S("No assignment operator for ") << v->type.type->identifier()
									<< S(", required from ") << owner->identifier());
					throw new (this) InternalError(msg);
				}

				*l << add(ptrA, ptrConst(v->offset()));
				*l << add(ptrC, ptrConst(v->offset()));
				*l << fnParam(ptr, ptrA);
				*l << fnParam(ptr, ptrC);
				*l << fnCall(ctor->ref(), true);
			} else {
				// Pointer or built-in.
				*l << mov(xRel(v->type.size(), ptrA, v->offset()), xRel(v->type.size(), ptrC, v->offset()));
			}
		}

		*l << fnRet(me);

		return t;
	}

	static Array<Value> *paramArray(Type *owner) {
		Array<Value> *v = new (owner) Array<Value>(2, thisPtr(owner));
		v->at(1) = thisPtr(CloneEnv::stormType(owner->engine));
		return v;
	}

	TypeDeepCopy::TypeDeepCopy(Type *owner)
		: Function(Value(), new (owner) Str(L"deepCopy"), paramArray(owner)),
		  owner(owner) {

		setCode(new (this) LazyCode(fnPtr(engine(), &TypeDeepCopy::generate, this)));
	}

	CodeGen *TypeDeepCopy::generate() {
		CodeGen *t = new (this) CodeGen(runOn(), true, Value());
		Listing *l = t->l;

		TypeDesc *ptr = engine().ptrDesc();
		Var me = l->createParam(ptr);
		Var env = l->createParam(ptr);

		*l << prolog();

		// Call the super-class (if possible).
		if (Type *super = owner->super()) {
			if (Function *before = super->deepCopyFn()) {
				*l << fnParam(ptr, me);
				*l << fnParam(ptr, env);
				*l << fnCall(before->directRef(), true);
			}
		}

		Package *core = engine().package(S("core"));

		Array<MemberVar *> *vars = owner->variables();
		for (Nat i = 0; i < vars->count(); i++) {
			MemberVar *var = vars->at(i);
			Value type = var->type;


			if (type.isPrimitive()) {
				// Nothing needs to be done for actors or built-in types.
			} else if (type.isValue()) {
				// No need to copy, just call 'deepCopy'.
				Function *toCall = type.type->deepCopyFn();
				if (!toCall) {
					// WARNING(L"No deepCopy function in " << type.type);
					continue;
				}

				*l << mov(ptrA, me);
				*l << add(ptrA, ptrConst(var->offset()));
				*l << fnParam(ptr, ptrA);
				*l << fnParam(ptr, env);
				*l << fnCall(toCall->ref(), true);
			} else if (type.isClass()) {
				// Call the system-wide copy function for this type.
				Function *toCall = as<Function>(core->find(S("clone"), paramArray(type.type), Scope()));
				if (!toCall)
					throw new (this) InternalError(TO_S(this, S("Can not find 'core.clone' for ") << type));

				*l << mov(ptrA, me);
				*l << fnParam(ptr, ptrRel(ptrA, var->offset()));
				*l << fnParam(ptr, env);
				*l << fnCall(toCall->ref(), false);
				*l << mov(ptrC, me);
				*l << mov(ptrRel(ptrC, var->offset()), ptrA);
			} else {
				// We don't need to be concerned with actors.
			}
		}

		*l << fnRet();

		return t;
	}

}
