#include "stdafx.h"
#include "Production.h"
#include "Compiler/Lib/Array.h"
#include "Compiler/Lib/Maybe.h"
#include "Compiler/TypeCtor.h"
#include "Core/Join.h"
#include "Utils/Memory.h"
#include "Exception.h"
#include "Node.h"
#include "SStr.h"
#include "Doc.h"
#include "Rule.h"
#include "Transform.h"
#include "Children.h"

namespace storm {
	namespace syntax {

		Production::Production() {
			tokens = new (this) Array<Token *>();
		}

		Production::Production(ProductionType *owner) {
			this->owner = owner;
			tokens = new (this) Array<Token *>();
		}

		Production::Production(ProductionType *owner, ProductionDecl *decl, Delimiters *delim, Scope scope) {
			this->owner = owner;
			parent = null;
			tokens = new (this) Array<Token *>();
			priority = decl->priority;
			repStart = decl->repStart;
			repEnd = decl->repEnd;
			repType = decl->repType;
			indentStart = decl->indentStart;
			indentEnd = decl->indentEnd;
			indentType = decl->indentType;
			result = decl->result;
			resultParams = decl->resultParams;

			if (decl->parent) {
				Named *p = scope.find(decl->parent);
				if (!p) {
					Str *msg = TO_S(this, S("The element ") << decl->parent << S(" was not found.")
									S(" It must refer to a rule."));
					throw new (this) SyntaxError(decl->pos, msg);
				}

				parent = as<Rule>(p);
				if (!parent) {
					Str *msg = TO_S(this, S("Parent elements must refer to a rule. ")
									<< decl->parent << S(" is a ") << parent << S("."));
					throw new (this) SyntaxError(decl->pos, msg);
				}
			}

			Nat counter = 0;
			if (decl->repCapture) {
				MemberVar *r = createTarget(decl->pos, Value(SStr::stormType(engine())), decl->repCapture, counter);
				if (r) {
					owner->add(r);
					repCapture = new (this) Token(decl->repCapture, r);
				}
			}

			for (nat i = 0; i < decl->tokens->count(); i++) {
				addToken(decl->tokens->at(i), delim, decl->pos, scope, counter);
			}
		}

		void Production::addToken(TokenDecl *decl, Delimiters *delim, SrcPos pos, Scope scope, Nat &counter) {
			Token *token = decl->create(pos, scope, delim);

			MemberVar *r = createTarget(pos, decl, token, tokens->count(), counter);
			if (r) {
				owner->add(r);
				token->update(decl, r);
			}

			// There might be a default color on this token, do not overwrite that if we do not have
			// anything better to say.
			if (decl->color != tNone)
				token->color = decl->color;

			tokens->push(token);
		}

		MAYBE(MemberVar *) Production::createTarget(SrcPos p, TokenDecl *decl, Token *token, Nat pos, Nat &counter) {
			Value type;
			if (token->asRegex()) {
				type = Value(SStr::stormType(engine()));
			} else if (RuleToken *rule = token->asRule()) {
				type = Value(rule->rule);
			} else {
				throw new (this) InternalError(TO_S(this, S("Unknown subtype of Token found: ") << token));
			}

			if (inRepeat(pos)) {
				if (repType == repZeroOne) {
					type = wrapMaybe(type);
				} else {
					type = wrapArray(type);
				}
			}

			return createTarget(p, type, decl, counter);
		}

		MAYBE(MemberVar *) Production::createTarget(SrcPos pos, Value type, TokenDecl *decl, Nat &counter) {
			if (decl->store) {
				return new (this) MemberVar(pos, decl->store, type, owner);
			} else if (decl->invoke) {
				StrBuf *name = new (this) StrBuf();
				*name << decl->invoke << (counter++);
				return new (this) MemberVar(pos, name->toS(), type, owner);
			} else {
				if (RuleTokenDecl *rule = as<RuleTokenDecl>(decl)) {
					if (rule->params && !rule->params->empty()) {
						// Also evaluate rule tokens which are given parameters (they may have desirable side effects).
						StrBuf *name = new (this) StrBuf();
						*name << L"<anon" << (counter++) << L">";
						return new (this) MemberVar(pos, name->toS(), type, owner);
					}
				}
				return null;
			}
		}

		Bool Production::inRepeat(Nat token) const {
			return repType != repNone
				&& repStart <= token
				&& repEnd > token;
		}

		Rule *Production::rule() const {
			if (owner)
				return owner->rule();
			else
				return null;
		}

		ProductionType *Production::type() const {
			return owner;
		}

		void Production::toS(StrBuf *to) const {
			toS(to, tokens->count() + 1);
		}

		void Production::toS(StrBuf *to, Nat mark) const {
			toS(to, mark, true);
		}

		void Production::toS(StrBuf *to, Nat mark, Bool bindings) const {
			if (parent)
				*to << parent->identifier() << S("..");

			if (Rule *r = rule())
				*to << r->identifier();
			else
				*to << S("?");

			if (priority != 0)
				*to << S("[") << priority << S("]");

			if (result) {
				*to << S(" => ") << result;
				if (resultParams) {
					*to << S("(");
					join(to, resultParams, S(", "));
					*to << S(")");
				}
			}

			*to << L" : ";

			bool usingRep = repStart < repEnd && repType != repNone;
			bool usingIndent = indentStart < indentEnd && indentType != indentNone;
			DelimToken *prevDelim = null;
			for (nat i = 0; i < tokens->count(); i++) {
				Token *token = tokens->at(i);
				DelimToken *currentDelim = token->asDelim();

				if (usingRep && repEnd == i)
					outputRepEnd(to, bindings);

				if (usingIndent && indentEnd == i)
					*to << S(" ]") << indentType;

				if (i > 0 && !currentDelim && !prevDelim)
					*to << S(" - ");

				if (usingIndent && indentStart == i)
					*to << S("[");

				if (usingRep && repStart == i)
					*to << S("(");

				if (i == mark)
					*to << S("<>");

				if (currentDelim) {
					switch (currentDelim->type) {
					case delim::optional:
						*to << S(", ");
						break;
					case delim::required:
						*to << S(" ~ ");
						break;
					}
				} else {
					token->toS(to, bindings);
				}

				prevDelim = currentDelim;
			}

			if (usingRep && repEnd == tokens->count())
				outputRepEnd(to, bindings);

			if (usingIndent && indentEnd == tokens->count())
				*to << S(" ]") << indentType;

			if (mark == tokens->count())
				*to << S("<>");

			if (owner)
				*to << S(" = ") << owner->name;
		}

		void Production::outputRepEnd(StrBuf *to, Bool bindings) const {
			*to << S(")");

			if (repType == repZeroOne) {
				*to << S("?");
			} else if (repType == repOnePlus) {
				*to << S("+");
			} else if (repType == repZeroPlus) {
				*to << S("*");
			} else if (repCapture) {
				repCapture->toS(to, bindings);
			}
		}

		/**
		 * Iterator.
		 */

		ProductionIter Production::firstA() {
			return ProductionIter(this, 0);
		}

		ProductionIter Production::firstB() {
			if (repStart == 0 && skippable(repType))
				return ProductionIter(this, repEnd);
			else
				return ProductionIter();
		}

		ProductionIter Production::posIter(Nat pos) {
			return ProductionIter(this, pos);
		}

		ProductionIter ProductionIter::nextA() const {
			return ProductionIter(p, pos + 1);
		}

		ProductionIter ProductionIter::nextB() const {
			Nat next = pos + 1;
			if (next == p->repStart) {
				// The start of something skippable -> skip it!
				if (skippable(p->repType))
					return ProductionIter(p, p->repEnd);
			} else if (next == p->repEnd) {
				// The end of a repetition -> repeat!
				if (repeatable(p->repType))
					return ProductionIter(p, p->repStart);
			}

			// Nothing possible, just return something invalid.
			return ProductionIter();
		}

		ProductionIter::ProductionIter() : p(null), pos(0) {}

		ProductionIter::ProductionIter(Production *p, Nat pos) : p(p), pos(pos) {}

		Bool ProductionIter::end() const {
			return p != null
				&& pos >= p->tokens->count();
		}

		Bool ProductionIter::valid() const {
			return p != null;
		}

		Bool ProductionIter::operator ==(const ProductionIter &o) const {
			return p == o.p
				&& pos == o.pos;
			// We ignore repCount and lie a little bit. This prevents infinite repetitions of zero-rules.
		}

		Bool ProductionIter::operator !=(const ProductionIter &o) const {
			return !(*this == o);
		}

		Bool ProductionIter::repStart() const {
			return pos == p->repStart;
		}

		Bool ProductionIter::repEnd() const {
			return pos == p->repEnd;
		}

		MAYBE(Rule *) ProductionIter::rule() const {
			if (p)
				return p->rule();
			else
				return null;
		}

		MAYBE(Production *) ProductionIter::production() const {
			return p;
		}

		MAYBE(Token *) ProductionIter::token() const {
			if (!end())
				return p->tokens->at(pos);
			else
				return null;
		}

		wostream &operator <<(wostream &to, const ProductionIter &i) {
			if (!i.valid())
				return to << L"<invalid>";
			StrBuf *b = new (i.production()) StrBuf();
			*b << i;
			return to << b->toS()->c_str();
		}

		void ProductionIter::toS(StrBuf *to) const {
			if (!valid())
				*to << S("<invalid>");
			else
				p->toS(to, pos, false);
		}


		/**
		 * Type
		 */

		ProductionType::ProductionType(Str *name, ProductionDecl *decl, Delimiters *delim, Scope scope)
			: Type(name, typeClass),
			  pos(decl->pos),
			  decl(decl),
			  scope(scope) {

			Rule *r = as<Rule>(scope.find(decl->rule));
			if (!r)
				throw new (this) SyntaxError(pos, TO_S(this, S("The rule ") << decl->rule << S(" was not found.")));
			setSuper(r);

			arrayMembers = new (this) Array<MemberVar *>();

			// Can not be created in the initializer list as it accesses us in 'add'.
			production = new (this) Production(this, decl, delim, scope);

			documentation = new (this) SyntaxDoc(decl->docPos, this);
		}

		ProductionType::ProductionType(SrcPos pos, Str *name, Rule *parent)
			: Type(name, typeClass),
			  pos(pos),
			  decl(null),
			  scope() {

			setSuper(parent);
			production = new (this) Production(this);
		}

		void ProductionType::add(Named *m) {
			Type::add(m);

			if (MemberVar *v = as<MemberVar>(m)) {
				// Only arrays are interesting.
				if (isArray(v->type))
					arrayMembers->push(v);
			}
		}

		Rule *ProductionType::rule() const {
			Rule *r = as<Rule>(super());
			if (!r)
				throw new (this) InternalError(S("An option does not inherit from Rule!"));
			return r;
		}

		void ProductionType::toS(StrBuf *to) const {
			*to << production << L"\n";
			Type::toS(to);
		}

		static void outputVar(Node *me, StrBuf *to, MemberVar *var) {
			if (!var)
				return;

			*to << var->name << L" : ";
			int offset = var->offset().current();
			if (isArray(var->type)) {
				Array<Object *> *v = OFFSET_IN(me, offset, Array<Object *> *);
				if (v)
					v->toS(to);
				else
					*to << L"null";
			} else {
				// It is a TObject or Object. Always on the same thread as us.
				TObject *v = OFFSET_IN(me, offset, TObject *);
				if (v)
					v->toS(to);
				else
					*to << L"null";
			}
			*to << L"\n";
		}

		// This is implemented in C++ since it does not really need to be fast, and would be painful
		// to implement in machine code.
		void CODECALL productionToS(Node *me, StrBuf *to) {
			// We know this is true, otherwise we would not be here...
			ProductionType *type = (ProductionType *)runtime::typeOf(me);
			Production *prod = type->production;

			*to << L"{ " << me->pos << L"\n";
			{
				Indent z(to);

				if (Rule *r = prod->rule())
					*to << r->identifier() << L" -> ";
				*to << type->identifier() << L"\n";

				for (nat i = 0; i < prod->tokens->count(); i++) {
					Token *t = prod->tokens->at(i);
					outputVar(me, to, t->target);
				}

				if (prod->repCapture)
					outputVar(me, to, prod->repCapture->target);
			}
			*to << L"}";
		}

		Bool ProductionType::loadAll() {
			Engine &e = engine;
			Value me = thisPtr(this);

			if (decl) {
				// Create toS.
				Value strBuf = thisPtr(StrBuf::stormType(e));
				Array<Value> *p = new (e) Array<Value>(2, me);
				p->at(1) = strBuf;
				add(nativeFunction(e, Value(), S("toS"), p, address(&productionToS)));

				// Load remaining functions.
				loadFunctions(decl, scope);

				// Clear!
				decl = null;
				scope = Scope();
			}

			if (!me.isActor()) {
				add(new (e) TypeCopyCtor(this));
				add(new (e) TypeDeepCopy(this));
			}

			return Type::loadAll();
		}

		void ProductionType::loadFunctions(ProductionDecl *decl, Scope scope) {
			add(createTransformFn(decl, this, scope));
			add(createChildrenFn(decl, this, scope));
		}

	}
}
