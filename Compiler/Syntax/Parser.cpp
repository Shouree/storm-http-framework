#include "stdafx.h"
#include "Parser.h"
#include "Package.h"
#include "Engine.h"
#include "Core/StrBuf.h"
#include "Earley/Parser.h"
#include "GLR/Parser.h"
#include "Lib/Parser.h"

namespace storm {
	namespace syntax {

#ifdef DEBUG
		bool parserDebug = false;
#endif

		Nat backendCount() {
			return 2;
		}

#define PARSER_BACKEND(id, name) case id: return new (e.v) name::Parser();

		ParserBackend *createBackend(EnginePtr e, Nat id) {
			switch (id) {
				PARSER_BACKEND(0, glr);
				PARSER_BACKEND(1, earley);
			default:
				throw new (e.v) InternalError(TO_S(e.v, S("No parser backend with id ") << id << S(" is known.")));
			}
		}


		ParserBase::ParserBase(ParserBackend *backend) {
			assert(as<ParserType>(runtime::typeOf(this)),
				L"ParserBase not properly constructed. Use Parser::create() in C++!");

			init(root(), backend);
		}

		ParserBase::ParserBase(Rule *root, ParserBackend *backend) {
			init(root, backend);
		}

		void ParserBase::init(Rule *root, ParserBackend *backend) {
			// TODO: Make it possible to specify which parser to use on creation!
			use = backend;
			if (!use)
				use = new (this) DEFAULT_PARSER::Parser();

			// Find the package where 'root' is located and add that!
			if (Package *pkg = ScopeLookup::firstPkg(root)) {
				addSyntax(pkg);
			} else {
				WARNING(L"The rule " << root << L" is not located in any package. No default package added!");
			}
		}

		void ParserBase::toS(StrBuf *to) const {
			// TODO: use 'toBytes' when present!
			*to << L"Parser for " << root()->identifier() << L", currently using " << stateCount()
				<< L" states = " << byteCount() << L" bytes.";
		}

		Nat ParserBase::stateCount() const {
			return use->stateCount();
		}

		Nat ParserBase::byteCount() const {
			return use->byteCount();
		}

		Rule *ParserBase::root() const {
			// This is verified in the constructor.
			ParserType *type = (ParserType *)runtime::typeOf(this);
			return type->root;
		}

		void ParserBase::addSyntaxI(Package *pkg) {
			pkg->forceLoad();
			for (NameSet::Iter i = pkg->begin(), e = pkg->end(); i != e; ++i) {
				Named *n = i.v();
				if (Rule *rule = as<Rule>(n)) {
					use->add(rule);
				} else if (ProductionType *t = as<ProductionType>(n)) {
					use->add(t);
				}
			}
		}

		void ParserBase::addSyntax(Package *pkg) {
			addSyntaxI(pkg);

			// Recursive exports.
			Array<Package *> *deps = pkg->recursiveExports();
			for (Nat i = 0; i < deps->count(); i++) {
				addSyntaxI(deps->at(i));
			}
		}

		void ParserBase::addSyntax(Array<Package *> *pkgs) {
			for (nat i = 0; i < pkgs->count(); i++)
				addSyntax(pkgs->at(i));
		}

		Bool ParserBase::sameSyntax(ParserBase *o) {
			return use->sameSyntax(o->use);
		}

		void ParserBase::clear() {
			use->clear();
		}

		Bool ParserBase::parse(Str *str, Url *file) {
			return parse(str, file, str->begin());
		}

		Bool ParserBase::parse(Str *str, Url *file, Str::Iter start) {
			return use->parse(root(), str, file, start);
		}

		Bool ParserBase::hasError() const {
			return use->hasError();
		}

		Bool ParserBase::hasTree() const {
			return use->hasTree();
		}

		Str::Iter ParserBase::matchEnd() const {
			return use->matchEnd();
		}

		Str *ParserBase::errorMsg() const {
			return use->errorMsg();
		}

		SyntaxError *ParserBase::error() const {
			return new (this) SyntaxError(use->errorPos(), errorMsg());
		}

		void ParserBase::throwError() const {
			if (hasError())
				throw error();
		}

		Node *ParserBase::tree() const {
			if (Node *r = use->tree())
				return r;
			throw error();
		}

		InfoNode *ParserBase::infoTree() const {
			if (InfoNode *r = unsafeInfoTree())
				return r;
			throw error();
		}


		/**
		 * Info parser.
		 */

		InfoParser::InfoParser(Rule *root) : ParserBase(root, null), rootRule(root) {}

		InfoParser::InfoParser(Rule *root, ParserBackend *backend) : ParserBase(root, backend), rootRule(root) {}

		InfoParser *InfoParser::create(Package *pkg, const wchar *name) {
			return create(pkg, name, null);
		}

		InfoParser *InfoParser::create(Package *pkg, const wchar *name, ParserBackend *backend) {
			Scope root = pkg->engine().scope();
			if (Rule *r = as<Rule>(pkg->find(name, root))) {
				return new (pkg) InfoParser(r, backend);
			} else {
				Str *msg = TO_S(pkg, S("Can not find the rule ") << name << S(" in ") << pkg->identifier());
				throw new (pkg) InternalError(msg);
			}
		}

		void InfoParser::root(Rule *r) {
			rootRule = r;
		}

		Rule *InfoParser::root() const {
			return rootRule;
		}

		Bool InfoParser::parse(Str *str, Url *file, Str::Iter start) {
			lastStr = str;
			lastOffset = start.offset();
			return ParserBase::parse(str, file, start);
		}

		InfoErrors InfoParser::parseApprox(Str *str, Url *file) {
			return parseApprox(str, file, str->begin(), null);
		}

		InfoErrors InfoParser::parseApprox(Str *str, Url *file, Str::Iter start) {
			return parseApprox(str, file, start, null);
		}

		InfoErrors InfoParser::parseApprox(Str *str, Url *file, InfoInternal *context) {
			return parseApprox(str, file, str->begin(), context);
		}

		InfoErrors InfoParser::parseApprox(Str *str, Url *file, Str::Iter start, InfoInternal *context) {
			lastStr = str;
			lastOffset = start.offset();
			return ParserBase::parseApprox(str, file, start, context);
		}


		void InfoParser::clear() {
			lastStr = null;
			lastOffset = 0;
		}

		InfoNode *InfoParser::fullInfoTree() {
			InfoNode *tree = unsafeInfoTree();
			Nat end = lastOffset;
			if (tree)
				end = tree->length() + lastOffset;

			assert(end <= lastStr->peekLength(),
				L"Parser is weird, returning a tree spanning too many characters.");
			if (end >= lastStr->peekLength())
				return tree;

			if (InfoInternal *i = as<InfoInternal>(tree)) {
				// Replace node with a new root node containing an additional leaf with the remaining data.
				InfoInternal *r = new (this) InfoInternal(i, i->count() + 1);
				Str::Iter from = lastStr->posIter(end);
				r->set(i->count(), new (this) InfoLeaf(null, lastStr->substr(from, lastStr->end())));
				return r;
			} else {
				// Replace everything with a large regex.
				Str::Iter from = lastStr->posIter(lastOffset);
				return new (this) InfoLeaf(null, lastStr->substr(from, lastStr->end()));
			}
		}

		/**
		 * C++ interface.
		 */

		Parser::Parser(ParserBackend *backend) : ParserBase(backend) {}

		Parser *Parser::create(Rule *root) {
			return create(root, null);
		}

		Parser *Parser::create(Rule *root, ParserBackend *backend) {
			// Pick an appropriate type for it (!= the C++ type).
			Type *t = parserType(root);
			void *mem = runtime::allocObject(sizeof(Parser), t);
			Parser *r = new (Place(mem)) Parser(backend);
			t->vtable()->insert(r);
			return r;
		}

		Parser *Parser::create(Package *pkg, const wchar *name) {
			return create(pkg, name, null);
		}

		Parser *Parser::create(Package *pkg, const wchar *name, ParserBackend *backend) {
			Scope root = pkg->engine().scope();
			if (Rule *r = as<Rule>(pkg->find(name, root))) {
				return create(r, backend);
			} else {
				Str *msg = TO_S(pkg->engine(), S("Can not find the rule ") << name << S(" in ") << pkg->identifier());
				throw new (pkg) InternalError(msg);
			}
		}

	}
}
