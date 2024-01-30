#include "stdafx.h"
#include "Function.h"
#include "Cast.h"
#include "Doc.h"
#include "Scope.h"
#include "Core/Fn.h"
#include "Compiler/Code.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Syntax/Parser.h"

namespace storm {
	namespace bs {
		using syntax::SStr;

		FunctionDecl *assignDecl(Scope scope,
								syntax::SStr *name,
								Array<NameParam> *params,
								syntax::Node *options,
								syntax::Node *body) {

			return new (name) FunctionDecl(scope, null, name, params, options, body);
		}



		FunctionDecl::FunctionDecl(Scope scope,
								MAYBE(SrcName *) result,
								syntax::SStr *name,
								Array<NameParam> *params,
								syntax::Node *options,
								syntax::Node *body)
			: scope(scope),
			  name(name),
			  result(result),
			  params(params),
			  options(options),
			  body(body) {}



		Named *FunctionDecl::doCreate() {
			Scope fScope = fileScope(scope, name->pos);

			Value result;
			if (this->result)
				result = fScope.value(this->result);

			// if (*name->v == S("asyncPostObject"))
			// 	PVAR(body);

			Array<ValParam> *par = bs::resolve(params, fScope);
			BSFunction *f = new (this) BSFunction(result, name, par, fScope, null, body);

			// Default thread?
			if (thread)
				f->thread(scope, thread);

			if (!this->result)
				f->make(fnAssign);

			if (options)
				syntax::transformNode<void, Scope, BSRawFn *>(options, fScope, f);

			return f;
		}

		MAYBE(Named *) bs::FunctionDecl::update(Scope scope) {
			NamePart *name = namePart();
			Named *found = scope.find(new (this) Name(name));
			if (BSFunction *bs = as<BSFunction>(found)) {
				update(bs);
			} else if (found) {
				// Can not update the function. We don't know it!
			} else {
				Named *c = create();
				resolve();
				return c;
			}

			return null;
		}

		void bs::FunctionDecl::update(BSFunction *fn) {
			Value result;
			if (this->result)
				result = scope.value(this->result);

			assert(fn->result == result);
			fn->update(bs::resolve(params, scope), body, name->pos);
		}

		NamePart *bs::FunctionDecl::namePart() const {
			return new (this) SimplePart(name->v, values(bs::resolve(params, scope)));
		}

		void bs::FunctionDecl::toS(StrBuf *to) const {
			*to << namePart();
		}


		/**
		 * Raw function.
		 */

		BSRawFn::BSRawFn(Value result, SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread)
			: Function(result, name->v, values(params)), valParams(params) {

			this->pos = name->pos;
			init(thread);
		}

		void BSRawFn::init(NamedThread *thread) {
			if (thread)
				runOn(thread);

			if (result.ref)
				throw new (this) SyntaxError(pos, S("Returning references is not a good idea!"));

			setCode(new (this) LazyCode(fnPtr(engine(), &BSRawFn::generateCode, this)));
		}

		void BSRawFn::thread(Scope scope, SrcName *name) {
			if (parentLookup)
				throw new (this) InternalError(S("Can not call BSRawFn:thread after the function is attached somewhere."));

			Scope fScope = fileScope(scope, name->pos);
			NamedThread *t = as<NamedThread>(fScope.find(name));
			if (!t)
				throw new (this) SyntaxError(name->pos, TO_S(engine(), name << S(" is not a named thread.")));
			runOn(t);
		}

		FnBody *BSRawFn::createBody() {
			throw new (this) InternalError(S("A BSRawFn can not be used without overriding 'createBody' or 'createRawBody'!"));
		}

		void BSRawFn::clearBody() {}

		CodeGen *BSRawFn::createRawBody() {
			FnBody *body = createBody();

			// Expression possibly wrapped around the body (casting the value if needed).
			Expr *bodyExpr = expectCastTo(body, result, Scope(this));
			// PLN(bodyExpr);

			// Generate code!
			using namespace code;
			CodeGen *state = new (this) CodeGen(runOn(), isMember(), result);

			Listing *l = state->l;

			*l << prolog();

			// Parameters
			for (Nat i = 0; i < valParams->count(); i++) {
				SimplePart *name = new (this) SimplePart(valParams->at(i).name);
				LocalVar *var = body->variable(name);
				assert(var);
				var->createParam(state);
			}

			if (result == Value()) {
				CodeResult *r = new (this) CodeResult();
				bodyExpr->code(state, r);
				state->returnValue(code::Var());
			} else {
				// If we can get the result as a reference, do that as we avoid a copy. The exception
				// is built-in types, as we can copy them cheaply.
				Value res = result;
				if (!res.isPrimitive() && bodyExpr->result().type().ref)
					res = result.asRef();

				CodeResult *r = new (this) CodeResult(res, l->root());
				bodyExpr->code(state, r);

				if (!bodyExpr->result().nothing()) {
					// If we get 'nothing', that means the result will not be produced at all.
					if (res.ref) {
						*state->l << fnRetRef(r->location(state));
					} else {
						state->returnValue(r->location(state));
					}
				}
			}

			// if (!identifier()->startsWith(S("lang.bs"))) {
			// if (identifier()->startsWith(S("tests.bs."))) {
			// 	PLN(bodyExpr);
			// 	PLN(identifier() << L": " << l);
			// 	// This can be used to see the transformed machine code as well:
			// 	PLN(engine().arena()->transform(l));
			// 	// See transformed machine code and binary output (similar to above).
			// 	// PVAR(new (this) Binary(engine().arena(), l, true));
			// }

			// PLN(bodyExpr);
			// PLN(identifier() << L": " << l);

			// Only dispose it now, when we know we're done (in theory, the backend could fail, but
			// this is generally good enough).
			if (!isInline) {
				clearBody();
				bodyCleared = true;
			}
			return state;
		}

		void BSRawFn::makeStatic() {
			if (parentLookup)
				throw new (this) InternalError(S("Do not call 'makeStatic' after adding the function to the name tree."));

			// Already static?
			if (fnFlags() & fnStatic)
				return;

			FnFlags outlawed = fnAbstract | fnFinal | fnOverride;
			if (fnFlags() & outlawed)
				throw new (this) SyntaxError(pos, S("Can not make functions marked abstract, final or override into static functions."));

			if (valParams->empty())
				return;
			if (!valParams->at(0).thisParam())
				return;

			// All seems well, proceed.
			make(fnStatic);
			valParams->remove(0);
			params->remove(0);
		}

		void BSRawFn::makeInline() {
			if (bodyCleared)
				throw new (this) InternalError(S("Do not call 'makeInline' after the function has been executed."));
			isInline = true;
			setCode(new (this) InlineCode(fnPtr(engine(), &BSRawFn::generateInlineCode, this)));
		}

		void BSRawFn::generateInlineCode(InlineParams params) {
			params.spillParams();

			FnBody *body = createBody();
			body->info->inlineResult = params.result;
			body->info->inlineLabel = params.state->l->label();
			body->info->inlineBlock = params.state->block;

			Expr *bodyExpr = expectCastTo(body, result, Scope(this));

			// Set parameters.
			for (Nat i = 0; i < valParams->count(); i++) {
				SimplePart *name = new (this) SimplePart(valParams->at(i).name);
				LocalVar *var = body->variable(name);
				assert(var);
				var->var = VarInfo(params.param(i).var());
			}

			bodyExpr->code(params.state, params.result->split(params.state));

			*params.state->l << body->info->inlineLabel;
			params.result->created(params.state);
		}

		CodeGen *BSRawFn::generateCode() {
			return createRawBody();
		}

		void BSRawFn::reset() {
			// Could be done better...
			if (isInline) {
				setCode(new (this) InlineCode(fnPtr(engine(), &BSRawFn::generateInlineCode, this)));
			} else {
				setCode(new (this) LazyCode(fnPtr(engine(), &BSRawFn::generateCode, this)));
			}
		}

		Array<LocalVar *> *BSRawFn::addParams(Block *to) {
			Array<LocalVar *> *params = new (this) Array<LocalVar *>();
			params->reserve(valParams->count());

			for (Nat i = 0; i < valParams->count(); i++) {
				LocalVar *v = createParam(engine(), valParams->at(i), pos);
				to->add(v);
				params->push(v);
			}

			return params;
		}

		Array<DocParam> *BSRawFn::docParams() {
			Array<DocParam> *r = new (this) Array<DocParam>();
			r->reserve(valParams->count());

			for (Nat i = 0; i < valParams->count(); i++)
				r->push(DocParam(valParams->at(i).name, valParams->at(i).type()));

			return r;
		}


		/**
		 * Function.
		 */

		BSFunction::BSFunction(Value result, SStr *name, Array<ValParam> *params, Scope scope,
								MAYBE(NamedThread *) thread, syntax::Node *body) :
			BSRawFn(result, name, params, thread), scope(scope), body(body) {}

		bs::FnBody *BSFunction::createBody() {
			if (!body)
				throw new (this) InternalError(TO_S(this, S("Multiple compilation of ") << identifier()));

			FnBody *result = syntax::transformNode<FnBody, BSFunction *>(body, this);
			return result;
		}

		void BSFunction::clearBody() {
			body = null;
		}

		Bool BSFunction::update(Array<ValParam> *params, syntax::Node *node, SrcPos pos) {
			if (valParams->count() != params->count())
				return false;
			for (Nat i = 0; i < params->count(); i++)
				if (valParams->at(i).type() != params->at(i).type())
					return false;

			valParams = params;
			this->body = node;
			this->pos = pos;
			reset();
			return true;
		}

		Bool BSFunction::update(Array<ValParam> *params, syntax::Node *node) {
			return update(params, node, this->pos);
		}

		Bool BSFunction::update(Array<Str *> *params, syntax::Node *body) {
			if (valParams->count() != params->count())
				return false;
			for (Nat i = 0; i < params->count(); i++)
				valParams->at(i).name = params->at(i);

			this->body = body;
			reset();
			return true;
		}

		Bool BSFunction::update(BSFunction *from) {
			return update(from->valParams, from->body);
		}


		/**
		 * Tree function.
		 */

		BSTreeFn::BSTreeFn(Value result, SStr *name, Array<ValParam> *params, MAYBE(NamedThread *) thread)
			: BSRawFn(result, name, params, thread) {}

		void BSTreeFn::body(FnBody *body) {
			root = body;
			reset();
		}

		bs::FnBody *BSTreeFn::createBody() {
			if (!root) {
				Str *msg = TO_S(engine(), S("The body of ") << identifier() << S(" was not set before trying to use it."));
				throw new (this) UsageError(msg);
			}

			return root;
		}

		void BSTreeFn::clearBody() {
			root = null;
		}


		/**
		 * Abstract function.
		 */

		BSAbstractFn::BSAbstractFn(Value result, SStr *name, Array<ValParam> *params)
			: BSRawFn(result, name, params, null) {

			// Just to make sure...
			makeAbstract();
		}

		CodeGen *BSAbstractFn::createRawBody() {
			using namespace code;

			CodeGen *g = new (this) CodeGen(runOn(), isMember(), result);

			// Parameters.
			for (Nat i = 0; i < valParams->count(); i++) {
				g->createParam(valParams->at(i).type());
			}

			*g->l << prolog();
			*g->l << fnParam(engine().ptrDesc(), objPtr(identifier()));
			*g->l << fnCall(engine().ref(builtin::throwAbstractError), false);

			// 'throwAbstractError' does not return, so just to be sure.
			*g->l << epilog();
			*g->l << ret(Size());

			return g;
		}


		/**
		 * Function body.
		 */

		bs::FnBody::FnBody(BSRawFn *owner, Scope scope)
			: ExprBlock(owner->pos, scope) {
			info = new (this) ReturnInfo(owner->result);
		 	parameters = owner->addParams(this);
		}

		bs::FnBody::FnBody(BSFunction *owner)
			: ExprBlock(owner->body ? owner->body->pos : owner->pos, owner->scope) {
			info = new (this) ReturnInfo(owner->result);
			parameters = owner->addParams(this);
		}

		void bs::FnBody::code(CodeGen *state, CodeResult *to) {
			initVariables(state);
			// We don't need to call the three-parameter version of 'blockCode'.
			blockCode(state, to);
		}

	}
}
