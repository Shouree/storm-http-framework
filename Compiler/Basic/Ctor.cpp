#include "stdafx.h"
#include "Ctor.h"
#include "Type.h"
#include "Cast.h"
#include "Named.h"
#include "Core/Fn.h"
#include "Compiler/Engine.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		BSRawCtor::BSRawCtor(Array<ValParam> *params, SrcPos pos)
			: Function(Value(), new (params) Str(Type::CTOR), values(params)),
			  params(params) {

			this->pos = pos;

			// If we inherit from a class that does not know which thread to run on, we need a Thread as the
			// first parameter (#1, it is after the this ptr).
			RunOn runOn = params->at(0).type().type->runOn();
			needsThread = runOn.state == RunOn::runtime;
			if (needsThread) {
				Value expected = thisPtr(Thread::stormType(engine()));

				if (params->count() < 2) {
					Str *msg = TO_S(engine(), S("This constructor needs to take a ")
									<< expected << S(" as the first parameter since we are threaded."));
					throw new (this) SyntaxError(pos, msg);
				}

				if (!expected.mayReferTo(params->at(1).type())) {
					Str *msg = TO_S(engine(), S("This constructor needs to take a ")
									<< expected << S(" as the first parameter, not " << params->at(1)));
					throw new (this) SyntaxError(pos, msg);
				}
			}

			// Rename until it is initialized!
			params->at(0).name = new (this) Str(L" this");
			reset();
		}

		void BSRawCtor::reset() {
			// Could be done better...
			setCode(new (this) LazyCode(fnPtr(engine(), &BSRawCtor::generateCode, this)));
		}

		code::Var BSRawCtor::findThread(CodeGen *s, Array<code::Operand> *params) {
			using namespace code;

			RunOn on = runOn();
			if (on.state != RunOn::runtime)
				return Function::findThread(s, params);

			// We know it is always the first parameter!
			code::Var r = s->l->createVar(s->block, Size::sPtr);
			*s->l << mov(r, params->at(1));
			return r;
		}

		CodeGen *BSRawCtor::generateCode() {
			CtorBody *body = createBody();
			body->checkInit();

			using namespace code;
			CodeGen *state = new (this) CodeGen(runOn(), true, Value());
			Listing *l = state->l;

			*l << prolog();

			// Parameters (first one is special).
			{
				code::Var thisVar = state->createParam(params->at(0).type());
				SimplePart *hiddenName = new (this) SimplePart(new (this) Str(L" this"));
				SimplePart *normalName = new (this) SimplePart(new (this) Str(L"this"));
				LocalVar *hidden = body->variable(hiddenName);
				LocalVar *normal = body->variable(normalName);
				hidden->var = VarInfo(thisVar);
				normal->var = VarInfo(thisVar);

				// Add variable info. In the debug info, we always give the this
				// variable. Otherwise, we would need additional temporary variables etc, which is
				// not really worth the trouble.
				Value thisVal = params->at(0).type();
				Listing::VarInfo *info = new (this) Listing::VarInfo(normalName->name, thisVal.type, thisVal.ref, pos);
				state->l->varInfo(thisVar, info);
			}

			for (nat i = 1; i < params->count(); i++) {
				// Value t = params->at(i).type;
				SimplePart *name = new (this) SimplePart(params->at(i).name);
				LocalVar *var = body->variable(name);
				assert(var);
				var->createParam(state);
			}

			CodeResult *r = new (this) CodeResult();
			body->code(state, r);

			*l << fnRet();

			// PLN(identifier() << L": " << l);
			// PLN(engine().arena()->transform(l, null));
			clearBody();
			return state;
		}

		CtorBody *BSRawCtor::createBody() {
			throw new (this) InternalError(S("A BSRawCtor can not be used without overriding 'createBody'!"));
		}

		void BSRawCtor::clearBody() {}

		void BSRawCtor::addParams(Block *to, LocalVar *&thread, Array<LocalVar *> *&paramsOut) {
			thread = null;

			paramsOut = new (this) Array<LocalVar *>();
			paramsOut->reserve(params->count());

			for (nat i = 0; i < params->count(); i++) {
				LocalVar *var = createParam(engine(), params->at(i), pos);
				to->add(var);

				if (i == 1 && needsThread)
					thread = var;
				else
					paramsOut->push(var);
			}
		}


		/**
		 * Regular ctor.
		 */

		BSCtor::BSCtor(Array<ValParam> *params, Scope scope, MAYBE(syntax::Node *) body, SrcPos pos)
			: BSRawCtor(params, pos), scope(scope), body(body) {}

		CtorBody *BSCtor::createBody() {
			if (!body)
				return defaultParse();

			return syntax::transformNode<CtorBody, BSCtor *>(body, this);
		}

		void BSCtor::clearBody() {
			body = null;
		}

		CtorBody *BSCtor::defaultParse() {
			CtorBody *r = new (this) CtorBody(this, scope);
			Actuals *actual = new (this) Actuals();
			InitBlock *super = new (this) InitBlock(pos, r, actual);
			super->pos = pos;
			r->add(super);
			return r;
		}


		/**
		 * Tree ctor.
		 */

		BSTreeCtor::BSTreeCtor(Array<ValParam> *params, SrcPos pos)
			: BSRawCtor(params, pos) {}

		void BSTreeCtor::body(CtorBody *body) {
			root = body;
			reset();
		}

		CtorBody *BSTreeCtor::createBody() {
			if (!root) {
				Str *msg = TO_S(engine(), S("The body of ") << identifier() << S("was not set before trying to use it."));
				throw new (this) UsageError(msg);
			}

			return root;
		}

		void BSTreeCtor::clearBody() {
			root = null;
		}


		/**
		 * Constructor body.
		 */

		static SrcPos pickPos(BSCtor *ctor) {
			if (ctor->body) {
				SrcPos pos = ctor->body->pos;
				pos.end++;
				return pos;
			} else {
				return ctor->pos;
			}
		}

		CtorBody::CtorBody(BSCtor *ctor) : ExprBlock(pickPos(ctor), ctor->scope), superCalled(false), initDone(false) {
			ctor->addParams(this, threadParam, parameters);
			owner = ctor;
		}

		CtorBody::CtorBody(BSRawCtor *ctor, Scope scope) : ExprBlock(ctor->pos, scope) {
			ctor->addParams(this, threadParam, parameters);
			owner = ctor;
		}

		void CtorBody::checkInit() {
			// Check if init and super calls are present!
			if (!initDone) {
				// Automatically insert a 'init' block at the end. That will call the super
				// constructor as well if it is necessary.
				try {
					add(new (this) InitBlock(pos, this, null));
				} catch (const SyntaxError *e) {
					StrBuf *msg = new (this) StrBuf();
					*msg << S("Failed to initialize the object implicitly: ");
					e->message(msg);
					*msg << S("\nInitialize the object manually with an 'init' block instead.");
					throw new (this) SyntaxError(e->pos, msg->toS());
				}
			}
		}

		void CtorBody::code(CodeGen *state, CodeResult *to) {
			initVariables(state);

			if (threadParam) {
				thread = state->l->createVar(state->block, Size::sPtr);
				*state->l << mov(thread, threadParam->var.v);
			}

			// We don't need to call the three-parameter version.
			blockCode(state, to);
		}


		/**
		 * SuperCall.
		 */

		SuperCall::SuperCall(SrcPos pos, CtorBody *block, Actuals *params) : Expr(pos), block(block) {
			if (block->superCalled) {
				throw new (this) SyntaxError(pos, S("Only one call to the super class constructor is allowed inside ")
											S("a constructor, and the call has to be done before the 'init' block."));
			}
			block->superCalled = true;

			// Add the regular 'this' parameter.
			thisVar = block->variable(new (this) SimplePart(S(" this")));
			thisPtr = thisVar->result;

			Type *super = thisPtr.type->super();
			if (!super)
				throw new (this) SyntaxError(pos, S("Can not call the super class constructor unless a parent ")
											S("class is present."));

			Value superPtr = thisPtr;
			superPtr.type = super;
			LocalVar *created = new (this) ThisVar(superPtr, thisVar->pos, true);
			block->add(created);

			this->params = params;
			LocalVarAccess *l = new (this) LocalVarAccess(pos, thisVar);
			params->addFirst(l);
		}

		ExprResult SuperCall::result() {
			return ExprResult();
		}

		void SuperCall::toS(StrBuf *to) const {
			*to << S("super") << params;
		}

		void SuperCall::code(CodeGen *s, CodeResult *r) {
			// Note: We already checked so that we have a super class.
			Type *parent = thisPtr.type->super();

			RunOn runOn = thisPtr.type->runOn();
			bool hiddenThread = runOn.state == RunOn::runtime;
			if (!hiddenThread && parent == TObject::stormType(engine())) {
				callTObject(s, runOn.thread);
				return;
			}

			// Find something to call.
			BSNamePart *values = new (this) BSNamePart(Type::CTOR, pos, params);
			values->alter(0, Value(parent));

			if (hiddenThread)
				values->insert(storm::thisPtr(Thread::stormType(engine())), 1);

			Function *ctor = as<Function>(parent->find(values, block->scope));
			if (!ctor) {
				Str *msg = TO_S(engine(), S("No constructor (") << values
								<< S(") found in ") << parent->identifier() << S("."));
				throw new (this) SyntaxError(pos, msg);
			}

			Array<code::Operand> *actuals = new (this) Array<code::Operand>();
			actuals->reserve(values->params->count());

			if (hiddenThread) {
				actuals->push(params->code(0, s, ctor->params->at(0), block->scope));
				actuals->push(block->threadParam->var.v);
				for (Nat i = 2; i < values->params->count(); i++)
					actuals->push(params->code(i - 1, s, ctor->params->at(i), block->scope));
			} else {
				for (Nat i = 0; i < values->params->count(); i++)
					actuals->push(params->code(i, s, ctor->params->at(i), block->scope));
			}

			CodeResult *t = new (this) CodeResult();
			ctor->localCall(s, actuals, t, false);
		}

		void SuperCall::callTObject(CodeGen *s, NamedThread *t) {
			if (params->expressions->count() != 1)
				throw new (this) SyntaxError(pos, S("Can not initialize a threaded object with parameters."));

			// Find the constructor of TObject.
			Type *parent = TObject::stormType(engine());
			Array<Value> *values = new (this) Array<Value>(2, Value(parent));
			values->at(1) = Value(Thread::stormType(engine()));
			Function *ctor = as<Function>(parent->find(Type::CTOR, values, block->scope));
			if (!ctor)
				throw new (this) InternalError(S("The constructor of TObject: __ctor(TObject, Thread) was not found!"));

			// Call the constructor.
			Array<code::Operand> *actuals = new (this) Array<code::Operand>();
			actuals->push(params->code(0, s, ctor->params->at(0), block->scope));
			actuals->push(t->ref());

			CodeResult *res = new (this) CodeResult();
			ctor->localCall(s, actuals, res, false);
		}


		/**
		 * Initializer.
		 */

		Initializer::Initializer(syntax::SStr *name, Expr *expr) : name(name), expr(expr) {}

		Initializer::Initializer(syntax::SStr *name, Actuals *params) : name(name), params(params) {}

		void Initializer::toS(StrBuf *to) const {
			*to << name->v;
			if (params) {
				*to << params;
			} else if (expr) {
				*to << S(" = ") << expr;
			}
		}


		/**
		 * InitBlock.
		 */

		InitBlock::InitBlock(SrcPos pos, CtorBody *block, MAYBE(Actuals *)params, Array<Initializer *> *init)
			: Expr(pos) {

			this->init(block, params);
			for (nat i = 0; i < init->count(); i++)
				this->init(init->at(i));
		}

		InitBlock::InitBlock(SrcPos pos, CtorBody *block, MAYBE(Actuals *) params) : Expr(pos) {
			init(block, params);
		}

		void InitBlock::init(CtorBody *block, MAYBE(Actuals *) params) {
			if (block->superCalled && params)
				throw new (this) SyntaxError(pos, S("It is not possible to call super twice by first calling 'super' ")
											S("and then 'init' with parameters. Either remove the call to 'super' or ")
											S("remove the parameters from the init block."));

			if (block->initDone)
				throw new (this) SyntaxError(pos, S("Only one init block is allowed in each constructor."));
			block->initDone = true;

			initializers = new (this) Array<Initializer *>();
			initialized = new (this) Set<Str *>();

			// Look at what is happening.
			thisVar = block->variable(new (this) SimplePart(S(" this")));
			thisPtr = thisVar->result;

			// See if we need to call the parent constructor.
			if (!block->superCalled && thisPtr.type->super()) {
				if (!params)
					params = new (this) Actuals();
				block->add(new (this) SuperCall(pos, block, params));
			}

			if (block->superCalled) {
				// Find the current 'this' variable and modify its type.
				LocalVar *created = block->variable(new (this) SimplePart(S("this")));
				created->result = thisPtr;
			} else {
				// We need to create it.
				LocalVar *created = new (this) ThisVar(thisPtr, thisVar->pos, true);
				created->constant = true;
				block->add(created);
			}

			scope = block->scope;
		}

		ExprResult InitBlock::result() {
			return ExprResult();
		}

		void InitBlock::init(Initializer *init) {
			Str *name = init->name->v;
			MemberVar *v = as<MemberVar>(thisPtr.type->find(name, new (this) Array<Value>(1, thisPtr), scope));
			if (v == null || v->params->at(0).type != thisPtr.type) {
				Str *msg = TO_S(engine(), S("The member variable ") << name << S(" was not found in ") << thisPtr);
				throw new (this) SyntaxError(init->name->pos, msg);
			}

			if (initialized->has(name)) {
				Str *msg = TO_S(engine(), S("The member variable ") << name << S(" has already been initialized."));
				throw new (this) SyntaxError(init->name->pos, msg);
			}

			initializers->push(init);
			initialized->put(name);
		}

		void InitBlock::toS(StrBuf *to) const {
			*to << S("init {\n");
			to->indent();

			for (Nat i = 0; i < initializers->count(); i++) {
				*to << initializers->at(i) << S(";\n");
			}

			to->dedent();
			*to << S("}");
		}

		Bool InitBlock::isolate() {
			return false;
		}

		// Protect (=make sure it is destroyed) the newly created value in the indicated variable if required.
		static void protect(CodeGen *s, code::Var object, code::Block varBlock, MemberVar *var) {
			Value type = var->type;
			if (type.isValue() && type.destructor() != code::Operand()) {
				code::Var v = s->l->createVar(varBlock, Size::sPtr, type.destructor(), code::freeOnException | code::freeInactive);
				*s->l << mov(v, object);
				*s->l << add(v, ptrConst(var->offset()));
				*s->l << activate(v);
			}
		}

		void InitBlock::code(CodeGen *s, CodeResult *r) {
			using namespace code;

			code::Var dest = thisVar->var.v;
			Type *type = thisPtr.type;

			// Block for member variable cleanups.
			code::Block varBlock = s->l->createBlock(s->block);
			*s->l << begin(varBlock);

			// From here on, we should set up the destructor to clear 'this' by calling its destructor
			// directly.
			if (Type *super = type->super()) {
				if (Function *fn = super->destructor()) {
					code::Var thisCleanup = s->l->createVar(varBlock, Size::sPtr, fn->directRef(), freeOnException);
					*s->l << mov(thisCleanup, dest);
				}
			}

			// Child scope for proper creation.
			CodeGen *child = s->child(varBlock);

			// Initialize any member variables that don't have explicit initialization first.
			Array<MemberVar *> *vars = type->variables();
			Map<Str *, MemberVar *> *names = new (this) Map<Str *, MemberVar *>();
			for (Nat i = 0; i < vars->count(); i++) {
				MemberVar *var = vars->at(i);
				if (initialized->has(var->name)) {
					names->put(var->name, var);
				} else {
					initVarDefault(child, var);
					protect(s, dest, varBlock, var);
				}
			}

			// Initialize the remaining variables in the order they appear in the declaration.
			for (Nat i = 0; i < initializers->count(); i++) {
				Initializer *init = initializers->at(i);
				MemberVar *var = names->get(init->name->v);

				initVar(child, var, init);
				protect(s, dest, varBlock, var);
			}

			// Remove all destructors here, since we can call the real destructor of 'this' now.
			*s->l << end(varBlock);

			// Set our VTable.
			if (type->typeFlags() & typeClass) {
				type->vtable()->insert(s->l, dest);
			}

			// From here on, we need to make sure that we're freeing our 'this' pointer properly.
			if (Function *fn = type->destructor()) {
				code::Var thisCleanup = s->l->createVar(s->block, Size::sPtr, fn->directRef(), freeOnException | freeInactive);
				*s->l << mov(thisCleanup, dest);
				*s->l << activate(thisCleanup);
			}
		}

		void InitBlock::initVarDefault(CodeGen *s, MemberVar *v) {
			using namespace code;

			Value t = v->type;
			code::Var dest = thisVar->var.v;

			if (t.ref) {
				Str *msg = TO_S(engine(), S("Can not initialize reference ") << v->name << S(", not implemented yet!"));
				throw new (this) SyntaxError(pos, msg);
			}

			if (v->initializer) {
				// If an initializer was specified, use that.
				initVarInitializer(s, v);
			} else if (t.isPrimitive()) {
				// Primitives don't need initialization since we zero memory by default.
			} else if (t.isValue()) {
				// Values:
				Function *ctor = t.type->defaultCtor();
				if (!ctor) {
					Str *msg = TO_S(engine(), S("Can not initialize ") << v->name << S(" by default-constructing it. ")
									S("Please initialize this member explicitly in ") << thisPtr << S("."));
					throw new (this) SyntaxError(pos, msg);
				}

				*s->l << mov(ptrA, dest);
				*s->l << add(ptrA, ptrConst(v->offset()));
				*s->l << fnParam(engine().ptrDesc(), ptrA);
				*s->l << fnCall(ctor->ref(), false);
			} else {
				// Classes:
				Function *ctor = t.type->defaultCtor();
				if (!ctor) {
					Str *msg = TO_S(engine(), S("Can not initialize ") << v->name << S(" by default-constructing it. ")
									S("Please initialize this member explicitly in ") << thisPtr << S("."));
					throw new (this) SyntaxError(pos, msg);
				}
				Actuals *params = new (this) Actuals();
				CtorCall *ctorCall = new (this) CtorCall(pos, scope, ctor, params);
				CodeResult *created = new (this) CodeResult(t, s->block);
				ctorCall->code(s, created);

				code::Var cVar = created->location(s);
				*s->l << mov(ptrA, dest);
				*s->l << mov(ptrRel(ptrA, v->offset()), cVar);
			}
		}

		void InitBlock::initVarInitializer(CodeGen *s, MemberVar *v) {
			using namespace code;

			assert(v->initializer);

			code::Var dest = thisVar->var.v;
			code::Var resultPtr = s->l->createVar(s->block, Size::sPtr);

			// Make a result pointer to pass to the function call.
			*s->l << mov(ptrA, dest);
			*s->l << add(ptrA, ptrConst(v->offset()));
			*s->l << mov(resultPtr, ptrA);

			// Call the function.
			v->initializer->autoCallRef(s, new (this) Array<code::Operand>(), resultPtr);
		}

		void InitBlock::initVar(CodeGen *s, MemberVar *v, Initializer *to) {
			using namespace code;

			Value t = v->type;

			if (t.ref) {
				Str *msg = TO_S(engine(), S("Can not initialize reference ") << v->name << S(", not implemented yet!"));
				throw new (this) SyntaxError(pos, msg);
			}

			assert(to->expr || to->params);

			if (to->expr) {
				Expr *init = castTo(to->expr, t, scope);
				if (t.isObject() && init) {
					initVarAssign(s, v, init);
				} else {
					Actuals *p = new (this) Actuals();
					p->add(to->expr);
					initVarCtor(s, v, p, false);
				}
			} else {
				initVarCtor(s, v, to->params, true);
			}

		}

		void InitBlock::initVarCtor(CodeGen *s, MemberVar *v, Actuals *to, Bool explicitCast) {
			using namespace code;

			Value t = v->type;
			code::Var dest = thisVar->var.v;
			Type *toCreate = t.type;

			BSNamePart *values = new (this) BSNamePart(Type::CTOR, pos, to);
			values->insert(storm::thisPtr(toCreate));

			Function *ctor = as<Function>(toCreate->find(values, scope));
			if (!ctor) {
				Str *msg = TO_S(engine(), S("No constructor for type '") << t << S("', ")
								<< S("required for initializing '") << v->name << S("'. ")
								<< S("Searched for: ") << values << S("."));
				throw new (this) SyntaxError(pos, msg);
			}

			if (!explicitCast && !implicitlyCallableCtor(ctor)) {
				Str *msg = TO_S(engine(), S("The constructor ") << ctor->shortIdentifier()
								<< S(" is not a copy constructor or marked with 'cast', and")
								<< S(" therefore needs to be called explicitly. If this was")
								<< S(" your intention, use \"") << v->name << S("(...);\" instead."));
				throw new (this) SyntaxError(pos, msg);
			}

			if (t.isObject()) {
				// Easy way, call the constructor as normal.
				CtorCall *call = new (this) CtorCall(pos, scope, ctor, to);
				CodeResult *created = new (this) CodeResult(t, s->block);
				call->code(s, created);
				code::Var loc = created->location(s);
				*s->l << mov(ptrA, dest);
				*s->l << mov(ptrRel(ptrA, v->offset()), loc);
				created->created(s);
			} else {
				// Now we're left with the values!

				Array<code::Operand> *actuals = new (this) Array<code::Operand>();
				actuals->reserve(values->params->count());
				// We will prepare 'ptrA' later.
				actuals->push(ptrA);
				for (nat i = 1; i < values->params->count(); i++)
					actuals->push(to->code(i - 1, s, ctor->params->at(i), scope));

				*s->l << mov(ptrA, dest);
				*s->l << add(ptrA, ptrConst(v->offset()));

				CodeResult *nothing = new (this) CodeResult();
				ctor->localCall(s, actuals, nothing, true);
			}
		}

		void InitBlock::initVarAssign(CodeGen *s, MemberVar *v, Expr *to) {
			using namespace code;

			Value t = v->type;
			code::Var dest = thisVar->var.v;
			assert(t.isObject());

			CodeResult *result = new (this) CodeResult(t, s->block);
			to->code(s, result);
			code::Var loc = result->location(s);
			*s->l << mov(ptrA, dest);
			*s->l << mov(ptrRel(ptrA, v->offset()), loc);
		}


		DelegateCall::DelegateCall(SrcPos pos, CtorBody *block, Actuals *params) : Expr(pos), block(block), params(params) {
			if (block->superCalled || block->initDone) {
				throw new (this) SyntaxError(pos, S("It is not possible to use 'self' together with either 'super'")
											S(" or 'init' constructs in the constructor."));
			}
			block->superCalled = true;
			block->initDone = true;

			// Look at the hidden "this" parameter.
			thisVar = block->variable(new (this) SimplePart(S(" this")));
			thisPtr = thisVar->result;

			// Add the regular "this" parameter for the remainder of the constructor.
			LocalVar *created = new (this) ThisVar(thisPtr, thisVar->pos, true);
			created->constant = true;
			block->add(created);

			// Update 'params'.
			this->params->addFirst(new (this) LocalVarAccess(pos, thisVar));
		}

		ExprResult DelegateCall::result() {
			return ExprResult();
		}

		void DelegateCall::code(CodeGen *s, CodeResult *r) {
			RunOn runOn = thisPtr.type->runOn();
			bool hiddenThread = runOn.state == RunOn::runtime;

			// Find something to call.
			BSNamePart *values = new (this) BSNamePart(Type::CTOR, pos, params);

			if (hiddenThread)
				values->insert(storm::thisPtr(Thread::stormType(engine())), 1);

			Function *ctor = as<Function>(thisPtr.type->find(values, block->scope));
			if (!ctor) {
				Str *msg = TO_S(engine(), S("No constructor (") << values
								<< S(") found in ") << thisPtr.type->identifier() << S("."));
				throw new (this) SyntaxError(pos, msg);
			}

			Array<code::Operand> *actuals = new (this) Array<code::Operand>();
			actuals->reserve(values->params->count());

			if (hiddenThread) {
				actuals->push(params->code(0, s, ctor->params->at(0), block->scope));
				actuals->push(block->threadParam->var.v);
				for (Nat i = 2; i < values->params->count(); i++)
					actuals->push(params->code(i - 1, s, ctor->params->at(i), block->scope));
			} else {
				for (Nat i = 0; i < values->params->count(); i++)
					actuals->push(params->code(i, s, ctor->params->at(i), block->scope));
			}

			CodeResult *t = new (this) CodeResult();
			ctor->localCall(s, actuals, t, false);
		}

		void DelegateCall::toS(StrBuf *to) const {
			*to << S("self") << params;
		}

		Bool DelegateCall::isolate() {
			return false;
		}

	}
}
