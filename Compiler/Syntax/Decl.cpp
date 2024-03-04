#include "stdafx.h"
#include "Decl.h"
#include "Core/Str.h"
#include "Core/Join.h"
#include "Core/CloneEnv.h"
#include "Exception.h"
#include "Compiler/Scope.h"
#include "Token.h"
#include "Rule.h"
#include "Production.h"

namespace storm {
	namespace syntax {

		/**
		 * FileItem.
		 */

		FileItem::FileItem() {}


		/**
		 * Use declarations.
		 */

		UseDecl::UseDecl(SrcName *pkg) : pkg(pkg) {}

		void UseDecl::deepCopy(CloneEnv *env) {
			cloned(pkg, env);
		}

		void UseDecl::toS(StrBuf *to) const {
			*to << S("use ") << pkg << S(";");
		}


		/**
		 * Delimiters.
		 */

		DelimDecl::DelimDecl(SrcName *token, delim::Delimiter type) : token(token), type(type) {}

		void DelimDecl::deepCopy(CloneEnv *env) {
			cloned(token, env);
		}

		void DelimDecl::toS(StrBuf *to) const {
			switch (type) {
			case delim::optional:
				*to << S("optional = ");
				break;
			case delim::required:
				*to << S("required = ");
				break;
			}

			*to << token << S(";");
		}

		DelimDecl *optionalDecl(SrcName *token) {
			return new (token) DelimDecl(token, delim::optional);
		}

		DelimDecl *requiredDecl(SrcName *token) {
			return new (token) DelimDecl(token, delim::required);
		}

		DelimDecl *allDecl(SrcName *token) {
			return new (token) DelimDecl(token, delim::all);
		}


		/**
		 * Parameters.
		 */

		ParamDecl::ParamDecl(Name *type, Str *name) : type(type), name(name) {}

		StrBuf &operator <<(StrBuf &to, ParamDecl decl) {
			return to << decl.type << L" " << decl.name;
		}


		/**
		 * Rules.
		 */

		RuleDecl::RuleDecl(SrcPos pos, Str *name, Name *result)
			: pos(pos), name(name), result(result), color(tNone) {
			params = new (this) Array<ParamDecl>();
		}

		void RuleDecl::deepCopy(CloneEnv *env) {
			cloned(pos, env);
			cloned(name, env);
			cloned(result, env);
			cloned(params, env);
		}

		void RuleDecl::toS(StrBuf *to) const {
			*to << result << S(" ") << name << S("(") << join(params, S(", ")) << S(")");

			if (color != tNone)
				*to << S(" #") << storm::syntax::name(engine(), color);
			*to << S(";");
		}

		void RuleDecl::push(Array<ParamDecl> *params) {
			this->params = params;
		}

		void RuleDecl::pushColor(SStr *color) {
			TokenColor c = tokenColor(color->v);
			if (c == tNone)
				throw new (this) SyntaxError(color->pos, S("Expected a color name."));
			this->color = c;
		}

		Rule *RuleDecl::create(Scope scope) {
			return new (this) Rule(this, scope);
		}


		/**
		 * Tokens.
		 */

		TokenDecl::TokenDecl() : color(tNone) {}

		void TokenDecl::deepCopy(CloneEnv *env) {
			cloned(store, env);
			cloned(invoke, env);
		}

		void TokenDecl::toS(StrBuf *to) const {
			if (raw)
				*to << S("@");
			if (store)
				*to << S(" ") << store;
			if (invoke)
				*to << S(" -> ") << invoke;
			if (color != tNone)
				*to << S(" #") << name(engine(), color);
		}

		void TokenDecl::pushRaw(Str *dummy) {
			raw = true;
		}

		void TokenDecl::pushStore(Str *store) {
			this->store = store;
		}

		void TokenDecl::pushInvoke(Str *invoke) {
			this->invoke = invoke;
		}

		void TokenDecl::pushColor(SStr *color) {
			TokenColor c = tokenColor(color->v);
			if (c == tNone)
				throw new (this) SyntaxError(color->pos, S("Expected a color name."));
			this->color = c;
		}

		Token *TokenDecl::create(SrcPos pos, Scope, Delimiters *) {
			throw new (this) SyntaxError(pos, S("Invalid token used!"));
		}

		Str *unescapeStr(Str *s) {
			return s->unescapeKeepBackslash(Char('"'));
		}


		/**
		 * Regex tokens.
		 */

		RegexTokenDecl::RegexTokenDecl(Str *regex) : regex(regex) {}

		void RegexTokenDecl::deepCopy(CloneEnv *env) {
			TokenDecl::deepCopy(env);
			cloned(regex, env);
		}

		void RegexTokenDecl::toS(StrBuf *to) const {
			*to << S("\"") << regex->escape(Char('"')) << S("\"");
			TokenDecl::toS(to);
		}

		Token *RegexTokenDecl::create(SrcPos pos, Scope, Delimiters *) {
			try {
				return new (this) RegexToken(regex);
			} catch (const RegexError *e) {
				throw new (this) SyntaxError(pos, e->message());
			}
		}

		/**
		 * Rule tokens.
		 */

		RuleTokenDecl::RuleTokenDecl(SrcPos pos, Name *rule) : pos(pos), rule(rule) {}

		RuleTokenDecl::RuleTokenDecl(SrcPos pos, Name *rule, Array<Str *> *params) :
			pos(pos), rule(rule), params(params) {}

		void RuleTokenDecl::deepCopy(CloneEnv *env) {
			TokenDecl::deepCopy(env);
			cloned(pos, env);
			cloned(rule, env);
			cloned(params, env);
		}

		void RuleTokenDecl::toS(StrBuf *to) const {
			*to << rule;
			if (params) {
				*to << S("(");
				join(to, params, S(", "));
				*to << S(")");
			}
			TokenDecl::toS(to);
		}

		Token *RuleTokenDecl::create(SrcPos pos, Scope scope, Delimiters *) {
			if (Rule *rule = as<Rule>(scope.find(this->rule))) {
				return new (this) RuleToken(rule, params);
			} else {
				throw new (this) SyntaxError(pos, TO_S(this, S("The rule ") << this->rule << S(" does not exist.")));
			}
		}

		/**
		 * Delimiter tokens.
		 */

		DelimTokenDecl::DelimTokenDecl(delim::Delimiter type) : type(type) {}

		void DelimTokenDecl::toS(StrBuf *to) const {
			switch (type) {
			case delim::optional:
				*to << S(", ");
				break;
			case delim::required:
				*to << S(" ~ ");
				break;
			}
		}

		Token *DelimTokenDecl::create(SrcPos pos, Scope, Delimiters *delim) {
			Rule *src = null;
			const wchar *name = S("");
			switch (type) {
			case delim::optional:
				src = delim->optional;
				name = S("optional");
				break;
			case delim::required:
				src = delim->required;
				name = S("required");
				break;
			}


			if (src) {
				return new (this) DelimToken(type, src);
			} else {
				throw new (this) SyntaxError(pos, TO_S(this, S("No ") << name << S(" delimiter was declared in this file.")));
			}
		}

		DelimTokenDecl *optionalTokenDecl(EnginePtr e) {
			return new (e.v) DelimTokenDecl(delim::optional);
		}

		DelimTokenDecl *requiredTokenDecl(EnginePtr e) {
			return new (e.v) DelimTokenDecl(delim::required);
		}


		SepTokenDecl::SepTokenDecl() {}

		void SepTokenDecl::toS(StrBuf *to) const {
			*to << S(" - ");
		}

		Token *SepTokenDecl::create(SrcPos pos, Scope, Delimiters *delim) {
			throw new (this) SyntaxError(pos, S("The SepTokenDecl should never be added to a production."));
		}


		/**
		 * Productions.
		 */

		ProductionDecl::ProductionDecl(SrcPos pos, Name *memberOf) : pos(pos), parent(null), rule(memberOf) {
			tokens = new (this) Array<TokenDecl *>();
			repType = repNone;
			indentType = indentNone;
		}

		ProductionDecl::ProductionDecl(SrcPos pos, Name *memberOf, MAYBE(Name *) parent)
			: pos(pos), parent(parent), rule(memberOf) {

			tokens = new (this) Array<TokenDecl *>();
			repType = repNone;
			indentType = indentNone;
		}

		void ProductionDecl::deepCopy(CloneEnv *env) {
			cloned(pos, env);
			cloned(parent, env);
			cloned(rule, env);
			cloned(tokens, env);
			cloned(name, env);
			cloned(result, env);
			cloned(resultParams, env);
			cloned(repCapture, env);
		}

		void ProductionDecl::toS(StrBuf *to) const {
			if (parent)
				*to << parent << S("..");

			*to << rule;
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

			bool usingRep = false;
			usingRep |= repType != repNone;
			usingRep |= repCapture != null;

			bool usingIndent = indentType != indentNone;

			*to << S(" : ");
			bool prevDelim = false;
			for (Nat i = 0; i < tokens->count(); i++) {
				TokenDecl *token = tokens->at(i);
				bool currentDelim = token->delimiter();

				if (usingRep && repEnd == i)
					outputRepEnd(to);

				if (usingIndent && indentEnd == i)
					*to << S(" ]") << indentType;

				if (i > 0 && !currentDelim && !prevDelim)
					*to << S(" - ");

				if (usingIndent && indentStart == i)
					*to << S("[");

				if (usingRep && repStart == i)
					*to << S("(");

				*to << token;

				prevDelim = currentDelim;
			}

			if (usingRep && repEnd == tokens->count())
				outputRepEnd(to);

			if (usingIndent && indentEnd == tokens->count())
				*to << S(" ]") << indentType;

			if (name)
				*to << S(" = ") << name;

			*to << S(";");
		}

		void ProductionDecl::outputRepEnd(StrBuf *to) const {
			*to << S(")");

			switch (repType) {
			case repZeroOne:
				*to << S("?");
				break;
			case repOnePlus:
				*to << S("+");
				break;
			case repZeroPlus:
				*to << S("*");
				break;
			case repNone:
				if (repCapture)
					*to << repCapture;
				break;
			}
		}

		void ProductionDecl::pushPrio(Int prio) {
			priority = prio;
		}

		void ProductionDecl::pushName(Str *name) {
			this->name = name;
		}

		void ProductionDecl::push(TokenDecl *decl) {
			if (as<SepTokenDecl>(decl))
				;
			else
				tokens->push(decl);
		}

		void ProductionDecl::pushResult(Name *result) {
			this->result = result;
		}

		void ProductionDecl::pushResult(Str *result) {
			this->result = new (this) Name(result);
		}

		void ProductionDecl::pushParams(Array<Str *> *p) {
			resultParams = p;
		}

		void ProductionDecl::pushRepStart(Str *dummy) {
			repStart = tokens->count();
		}

		void ProductionDecl::pushRepEnd(RepType type) {
			repEnd = tokens->count();
			repType = type;
		}

		void ProductionDecl::pushRepEnd(TokenDecl *capture) {
			repEnd = tokens->count();
			repType = repNone;
			repCapture = capture;
		}

		void ProductionDecl::pushIndentStart(Str *dummy) {
			indentStart = tokens->count();
		}

		void ProductionDecl::pushIndentEnd(IndentType type) {
			indentEnd = tokens->count();
			indentType = type;
		}

		ProductionType *ProductionDecl::create(Package *into, Delimiters *delimiters, Scope scope) {
			Str *name = this->name;
			if (!name)
				name = into->anonName();
			return new (this) ProductionType(name, this, delimiters, scope);
		}


		/**
		 * Custom declaration.
		 */

		CustomDecl::CustomDecl() {}

		void CustomDecl::expand(FileContents *to) {}


		/**
		 * File contents.
		 */

		FileContents::FileContents() {
			use = new (this) Array<SrcName *>();
			rules = new (this) Array<RuleDecl *>();
			productions = new (this) Array<ProductionDecl *>();
		}

		void FileContents::push(FileItem *item) {
			if (RuleDecl *r = as<RuleDecl>(item))
				rules->push(r);
			else if (ProductionDecl *p = as<ProductionDecl>(item))
				productions->push(p);
			else if (UseDecl *u = as<UseDecl>(item))
				use->push(u->pkg);
			else if (DelimDecl *d = as<DelimDecl>(item))
				pushDelimiter(d);
			else if (CustomDecl *c = as<CustomDecl>(item))
				c->expand(this);
			else
				WARNING(L"Unknown FileItem!");
		}

		void FileContents::pushDelimiter(DelimDecl *decl) {
			switch (decl->type) {
			case delim::all:
				optionalDelimiter = decl->token;
				requiredDelimiter = decl->token;
				break;
			case delim::optional:
				optionalDelimiter = decl->token;
				break;
			case delim::required:
				requiredDelimiter = decl->token;
				break;
			}
		}

		Delimiters *FileContents::delimiters(Scope scope) {
			Delimiters *r = new (this) Delimiters();
			if (optionalDelimiter)
				r->optional = resolveDelimiter(scope, optionalDelimiter);
			if (requiredDelimiter)
				r->required = resolveDelimiter(scope, requiredDelimiter);
			return r;
		}

		Rule *FileContents::resolveDelimiter(Scope scope, SrcName *name) {
			Named *found = scope.find(name);
			if (!found)
				throw new (this) SyntaxError(name->pos,
											TO_S(this, S("The name \"") << name << S("\" does not exist.")));

			if (Rule *r = as<Rule>(found))
				return r;

			throw new (this) SyntaxError(name->pos,
										TO_S(this, S("The name \"") << name << S("\" does not refer to a rule.")));
		}

		void FileContents::deepCopy(CloneEnv *env) {
			cloned(use, env);
			cloned(optionalDelimiter, env);
			cloned(requiredDelimiter, env);
			cloned(rules, env);
			cloned(productions, env);
		}

		void FileContents::toS(StrBuf *to) const {
			for (nat i = 0; i < use->count(); i++)
				*to << S("\nuse ") << use->at(i);

			if (optionalDelimiter)
				*to << S("\noptional delimiter = ") << optionalDelimiter;
			if (requiredDelimiter)
				*to << S("\nrequired delimiter = ") << requiredDelimiter;

			if (rules->any())
				*to << S("\n");
			for (nat i = 0; i < rules->count(); i++)
				*to << S("\n") << rules->at(i);

			if (productions->any())
				*to << S("\n");
			for (nat i = 0; i < productions->count(); i++)
				*to << S("\n") << productions->at(i);
		}

		Str *joinName(Str *first, Array<Str *> *rest) {
			StrBuf *to = new (first) StrBuf();
			*to << first;

			for (Nat i = 0; i < rest->count(); i++)
				*to << S(".") << rest->at(i);

			return to->toS();
		}

		RuleDecl *applyDoc(SrcPos doc, RuleDecl *decl) {
			decl->docPos = doc;
			return decl;
		}

		ProductionDecl *applyDoc(SrcPos doc, ProductionDecl *decl) {
			decl->docPos = doc;
			return decl;
		}

		FileItem *applyDoc(SrcPos doc, FileItem *decl) {
			if (RuleDecl *rule = as<RuleDecl>(decl))
				applyDoc(doc, rule);
			else if (ProductionDecl *prod = as<ProductionDecl>(decl))
				applyDoc(doc, prod);

			return decl;
		}

	}
}
