#include "stdafx.h"
#include "Class.h"
#include "Compiler/Exception.h"
#include "Compiler/TypeCtor.h"
#include "Compiler/TypeDtor.h"
#include "Compiler/Engine.h"
#include "Function.h"
#include "Ctor.h"
#include "Access.h"
#include "Doc.h"
#include "VariableInitializer.h"

namespace storm {
	namespace bs {

		Class *createClass(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body) {
			return new (name) Class(typeClass, pos, env, name->v, body);
		}

		Class *createValue(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body) {
			return new (name) Class(typeValue, pos, env, name->v, body);
		}


		Class::Class(TypeFlags flags, SrcPos pos, Scope scope, Str *name, syntax::Node *body)
			: Type(name, flags), scope(scope, this), body(body),
			  superName(null), threadName(null), threadMeaning(threadNone), allowLazyLoad(true) {

			this->pos = pos;
		}


		void Class::lookupTypes() {
			allowLazyLoad = false;

			// Lookup super-types and threads relative to the *parent* of the current scope,
			// otherwise we resolve them relative to the body of the class we are defining, which
			// does not make sense. It also unneccessary triggers the 'loadAll' in the parent in
			// case we have both a super-class and a thread set (e.g. when one does 'on Compiler:'
			// globally in the file). This could make loading happen in an incorrect order, and
			// cause issues that are difficult to track down.
			Scope scope = this->scope;
			if (scope.top)
				scope.top = scope.top->parent();

			try {
				// Set the super class first, this makes 'setThread' return true or false as appropriate:
				if (superName) {
					Type *t = as<Type>(scope.find(superName));
					if (!t) {
						Str *msg = TO_S(engine, S("Can not find the super class ") << superName << S("."));
						throw new (this) SyntaxError(superName->pos, msg);
					}
					setSuper(t);
				}

				if (threadMeaning != threadNone) {
					if (threadName == null) {
						if (superName) {
							throw new (this) SyntaxError(
								superName->pos,
								S("Can not use 'on ?' together with the 'extends' keyword."));
						}

						setSuper(TObject::stormType(engine));
					} else {
						NamedThread *t = as<NamedThread>(scope.find(threadName));
						if (!t) {
							Str *msg = TO_S(engine, S("Can not find the named thread ") << threadName << S("."));
							throw new (this) SyntaxError(threadName->pos, msg);
						}

						if (!setThread(t) && threadMeaning != threadDefault) {
							Str *msg = TO_S(engine, S("Failed to apply the 'on' keyword for this class since ")
											S("it inherits from a class type, or from an actor that is ")
											S("already bound to a thread."));
							throw new (this) SyntaxError(threadName->pos, msg);
						}
					}
				}

				threadName = null;
				threadMeaning = threadNone;
				superName = null;
			} catch (...) {
				allowLazyLoad = true;
				throw;
			}
			allowLazyLoad = true;
		}

		void Class::super(SrcName *super) {
			if (superName)
				throw new (this) SyntaxError(super->pos, S("Only one instance of 'extends' may be used for a single type. ")
											S("Multiple inheritance is not supported."));

			superName = super;
		}

		void Class::thread(SrcName *thread) {
			if (threadMeaning != threadNone && threadMeaning != threadDefault)
				throw new (this) SyntaxError(thread->pos, S("The 'on' keyword may only be used once."));

			threadName = thread;
			threadMeaning = threadExplicit;
		}

		void Class::unknownThread(SrcPos pos) {
			if (threadMeaning != threadNone && threadMeaning != threadDefault)
				throw new (this) SyntaxError(pos, S("The 'on' keyword may only be used once."));

			threadName = null;
			threadMeaning = threadExplicit;
		}

		void Class::defaultThread(SrcName *thread) {
			// Just ignore the default if it is already set!
			if (threadMeaning != threadNone)
				return;

			threadName = thread;
			threadMeaning = threadDefault;
		}

		void Class::decorate(SrcName *decorator) {
			if (!decorators)
				decorators = new (this) Array<SrcName *>();

			decorators->push(decorator);
		}

		void Class::makeFinal() {
			addTypeFlag(typeFinal);
		}

		Class::AddState::AddState() : ctor(false), copyCtor(false), deepCopy(false), assign(false) {}

		bool Class::loadAll() {
			if (!allowLazyLoad)
				return false;

			// Make sure that 'lookupTypes' was called. This might not be true in case some type
			// refers to this type inside its super type or something like that. It is fine to do
			// this, since 'lookupTypes' is a no-op if it was called already.
			lookupTypes();

			ClassBody *body = syntax::transformNode<ClassBody, Class *>(this->body, this);
			body->prepareItems();

			AddState added;

			// Add the named objects first.
			for (Nat i = 0; i < body->items->count(); i++) {
				addMember(body->items->at(i), added);
			}

			// Poke the classes to tell them they can load their super types now.
			for (Nat i = 0; i < body->items->count(); i++) {
				if (Class *c = as<Class>(body->items->at(i)))
					c->lookupTypes();
			}

			body->prepareWraps();

			// Add the wrapped items.
			for (Nat i = 0; i < body->wraps->count(); i++) {
				addMember(body->wraps->at(i)->transform(this), added);
			}


			// Add default members as required.
			if (needsDestructor(this))
				add(new (engine) TypeDefaultDtor(this));

			if (!added.ctor)
				add(classDefaultCtor(this));

			if (!added.copyCtor && runOn().state == RunOn::any)
				add(new (engine) TypeCopyCtor(this));

			if (!added.deepCopy && runOn().state == RunOn::any)
				add(new (engine) TypeDeepCopy(this));

			if ((typeFlags() & typeValue) && !added.assign)
				add(new (engine) TypeAssign(this));

			for (Nat i = 0; i < body->templates->count(); i++) {
				add(body->templates->at(i));
			}

			// Call any decorators.
			if (decorators) {
				for (Nat i = 0; i < decorators->count(); i++) {
					SrcName *name = decorators->at(i);
					Name *params = name->parent();
					params->add(new (this) SimplePart(name->last()->name, Value(Class::stormType(engine))));

					Function *found = as<Function>(scope.find(params));
					if (!found) {
						Str *msg = TO_S(engine, S("Could not find a decorator named ")
										<< name << S(" in the current scope."));
						throw new (this) SyntaxError(name->pos, msg);
					}

					if (found->result != Value())
						throw new (this) SyntaxError(name->pos, S("Decorators may not return a value."));

					// Call the function...
					typedef void (*Fn)(Class *);
					Fn fn = (Fn)found->pointer();
					(*fn)(this);
				}
			}

			// Done loading!
			body->finished();

			// We do not need the syntax tree anymore!
			this->body = null;

			// Super needs to be called!
			return Type::loadAll();
		}

		void Class::addMember(Named *item, AddState &state) {
			if (*item->name == Type::CTOR) {
				if (item->params->count() == 2 && item->params->at(0) == item->params->at(1))
					state.copyCtor = true;
				state.ctor = true;
			} else if (*item->name == S("deepCopy")) {
				if (item->params->count() == 2 && item->params->at(1).type == CloneEnv::stormType(this))
					state.deepCopy = true;
			} else if (*item->name == S("=")) {
				if (item->params->count() == 2 && item->params->at(1).type == this)
					state.assign = true;
			}

			add(item);
		}


		/**
		 * Wrap.
		 */

		MemberWrap::MemberWrap(syntax::Node *node) : node(node), visibility(null) {}

		Named *MemberWrap::transform(Class *owner) {
			Named *n = syntax::transformNode<Named, Class *>(node, owner);
			if (visibility)
				apply(node->pos, n, visibility);
			if (docPos.any())
				applyDoc(docPos, n);
			return n;
		}


		/**
		 * Body.
		 */

		ClassBody::ClassBody(Class *owner) : owner(owner) {
			items = new (this) Array<Named *>();
			wraps = new (this) Array<MemberWrap *>();
			templates = new (this) Array<Template *>();

			// TODO: Default to 'private' instead?
			defaultVisibility = engine().visibility(Engine::vPublic);
		}

		void ClassBody::add(Named *i) {
			if (!i->visibility)
				i->visibility = defaultVisibility;
			items->push(i);
		}

		void ClassBody::add(MemberWrap *w) {
			if (!w->visibility)
				w->visibility = defaultVisibility;
			wraps->push(w);
		}

		void ClassBody::add(Template *t) {
			templates->push(t);
		}

		void ClassBody::add(Visibility *v) {
			defaultVisibility = v;
		}

		void ClassBody::add(TObject *o) {
			if (Named *n = as<Named>(o)) {
				add(n);
			} else if (MemberWrap *w = as<MemberWrap>(o)) {
				add(w);
			} else if (Template *t = as<Template>(o)) {
				add(t);
			} else if (Visibility *v = as<Visibility>(o)) {
				add(v);
			} else {
				Str *msg = TO_S(this, S("Not a suitable type to ClassBody.add(): ")
								<< runtime::typeOf(o)->identifier());
				throw new (this) InternalError(msg);
			}
		}

		void ClassBody::prepareItems() {}

		void ClassBody::prepareWraps() {}

		void ClassBody::finished() {}

		/**
		 * Members.
		 */

		MemberVar *classVar(Class *owner, SrcName *type, syntax::SStr *name) {
			return classVar(owner, type, name, null);
		}

		MemberVar *classVar(Class *owner, SrcName *type, syntax::SStr *name, MAYBE(syntax::Node *) init) {
			Value t = owner->scope.value(type);
			MemberVar *var = new (owner) MemberVar(name->pos, name->v, t, owner);

			if (init) {
				var->initializer = new (owner) VariableInitializer(name->pos, t, owner->scope, init);
				var->initializer->make(fnStatic);
				var->initializer->parentLookup = var;
			}

			return var;
		}


		BSFunction *classFn(Class *owner,
							SrcPos pos,
							syntax::SStr *name,
							Name *result,
							Array<NameParam> *params,
							syntax::Node *body) {

			return new (owner) BSFunction(owner->scope.value(result, pos),
										name,
										resolve(params, owner, owner->scope),
										owner->scope,
										null, // thread
										body);
		}

		Function *classAbstractFn(Class *owner,
								SrcPos pos,
								syntax::SStr *name,
								Name *result,
								Array<NameParam> *params,
								syntax::Node *options) {

			BSRawFn *f = new (owner) BSAbstractFn(owner->scope.value(result, pos),
												name,
												resolve(params, owner, owner->scope));

			syntax::transformNode<void, Class *, BSRawFn *>(options, owner, f);

			if (f->fnFlags() & fnAbstract)
				return f;

			throw new (owner) SyntaxError(pos, S("A function without implementation must be marked using ': abstract'."));
		}

		BSFunction *classAssign(Class *owner,
								SrcPos pos,
								syntax::SStr *name,
								Array<NameParam> *params,
								syntax::Node *body) {

			BSFunction *f = new (owner) BSFunction(Value(),
												name,
												resolve(params, owner, owner->scope),
												owner->scope,
												null, // thread
												body);
			f->make(fnAssign);
			return f;
		}

		BSCtor *classCtor(Class *owner,
						SrcPos pos,
						Array<NameParam> *params,
						syntax::Node *body) {

			return new (owner) BSCtor(resolve(params, owner, owner->scope),
									owner->scope,
									body,
									pos);
		}

		BSCtor *classCastCtor(Class *owner,
							SrcPos pos,
							Array<NameParam> *params,
							syntax::Node *body) {

			BSCtor *r = classCtor(owner, pos, params, body);
			r->makeAutoCast();
			return r;
		}


		BSCtor *classDefaultCtor(Class *owner) {
			Array<ValParam> *params = new (owner) Array<ValParam>();

			Bool needsThread = owner->runOn().state == RunOn::runtime;
			if (needsThread) {
				params->reserve(2);
				params->push(thisParam(owner));
				params->push(ValParam(StormInfo<Thread>::type(owner->engine), new (owner) Str()));
			} else {
				params->reserve(1);
				params->push(thisParam(owner));
			}

			return new (owner) BSCtor(params, owner->scope, null, owner->pos);
		}

	}
}
