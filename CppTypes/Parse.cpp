#include "stdafx.h"
#include "Parse.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Utils/TextReader.h"
#include "Utils/FileStream.h"

/**
 * Namespace to add stuff to the global World.
 */
class WorldNamespace : public Namespace {
public:
	WorldNamespace(World &to, const CppName &prefix) : to(to), prefix(prefix) {}

	World &to;
	CppName prefix;

	void add(const Variable &v) {
		// Add stuff to 'world'.
		// PLN(L"Global variable " << v << L" ignored.");
		// TODO: Support globals!
	}

	void add(const Function &f) {
		Function g = f;
		g.name = prefix + f.name;
		to.functions.push_back(g);
	}
};

/**
 * Parsing environment.
 */
struct ParseEnv {
	// Current package.
	String pkg;

	// Export types in here?
	bool exportAll;

	// The world in which we want to store everything.
	World &world;

	// Last documentation comment seen here.
	Auto<Doc> doc;
};

// Get the current documentation.
static Auto<Doc> getDoc(Tokenizer &tok, ParseEnv &env) {
	Comment r = tok.comment();
	tok.clearComment();
	if (r.empty())
		return env.doc;

	env.doc = new Doc(r);
	return env.doc;
}

// Read a name.
static CppName parseName(Tokenizer &tok) {
	std::wostringstream out;
	out << tok.next().token;

	while (tok.more() && tok.skipIf(L"::")) {
		out << L"::" << tok.next().token;
	}

	return CppName(out.str());
}

// Read a type.
static Auto<TypeRef> parseTypeRef(Tokenizer &tok) {
	bool isConst = false;

	if (tok.skipIf(L"const"))
		isConst = true;
	tok.skipIf(L"volatile"); // Just ignore 'volatile' so far.
	tok.skipIf(L"mutable"); // Just ignore 'mutable' -- we do not worry about const.

	Auto<TypeRef> type;
	if (tok.skipIf(L"MAYBE")) {
		tok.expect(L"(");
		type = new MaybeClassType(parseTypeRef(tok));
		tok.expect(L")");
	} else if (tok.skipIf(L"UNKNOWN")) {
		tok.expect(L"(");
		CppName kind = parseName(tok);
		tok.expect(L")");
		type = new UnknownType(kind, parseTypeRef(tok));
	} else {
		SrcPos pos = tok.peek().pos;
		CppName n = parseName(tok);

		if (tok.skipIf(L"<")) {
			// Template parameters.
			Auto<TemplateType> t = new TemplateType(pos, n);

			if (!tok.skipIf(L">")) {
				t->params.push_back(parseTypeRef(tok));
				while (tok.skipIf(L","))
					t->params.push_back(parseTypeRef(tok));
				tok.expect(L">");
			}

			type = t;
		} else {
			type = new NamedType(pos, n);
		}
	}

	if (isConst)
		type->constType = true;

	while (tok.more()) {
		if (tok.skipIf(L"const"))
			type->constType = true;

		Token modifiers = tok.peek();
		bool wrong = false;
		Auto<TypeRef> nType = type;
		for (nat i = 0; i < modifiers.token.size(); i++) {
			if (modifiers.token[i] == '*') {
				nType = new PtrType(nType);
			} else if (modifiers.token[i] == '&') {
				nType = new RefType(nType);
			} else {
				wrong = true;
			}
		}

		if (wrong)
			break;

		tok.next();
		type = nType;
	}

	return type;
}

// Parse an unknown block.
static void parseBlock(Tokenizer &tok) {
	while (tok.more()) {
		tok.clearComment();
		Token t = tok.next();
		if (t.token == L"{")
			parseBlock(tok);
		else if (t.token == L"}")
			break;
	}
}

// Skip an empty array modifier (eg. int foo[]) if applicable.
static void skipArray(Tokenizer &tok, bool skippable) {
	if (tok.skipIf(L"[")) {
		if (skippable)
			tok.expect(L"]");
		else
			throw Error(L"Can not use the array syntax on parameters here. Use regular pointers instead.", tok.peek().pos);
	}
}

// Skip an ALIGN_AS definition if present.
static void skipAlignAs(Tokenizer &tok) {
	if (tok.skipIf(L"ALIGN_AS")) {
		tok.expect(L"(");
		nat level = 1;
		while (level > 0) {
			Token t = tok.next();
			if (t.token == L"(")
				level++;
			else if (t.token == L")")
				level--;
		}
	}
}

// Skip an initializer list for a constructor. Assumes we have just consumed the ":".
static void skipInitList(Tokenizer &tok) {
	do {
		tok.skip(); // variable name to initialize.

		nat parens = 0;
		nat braces = 0;
		Token t = tok.next();
		if (t.token == L"(") {
			parens++;
		} else if (t.token == L"{") {
			braces++;
		} else {
			throw Error(L"Expected ( or {, but got " + t.token + L".", t.pos);
		}

		// This is the tricky part. We don't really want to attempt to parse arbitrary C++
		// expressions. Thus, we fall back to counting the number of () and {} and end when we find a
		// corresponding closing tag for the start token. We don't bother to make sure they are matched:
		// the C++ compiler will do that anyway. As long as we manage to skip correct initializer lists
		// that are not too complicated, we're happy.
		while (parens > 0 && parens > 0) {
			Token t = tok.next();
			if (t.token == L"(") {
				parens++;
			} else if (t.token == L"{") {
				braces++;
			} else if (t.token == L"}") {
				if (braces > 0)
					braces--;
				else
					throw Error(L"Unexpected }.", t.pos);
			} else if (t.token == L")") {
				if (parens > 0)
					parens--;
				else
					throw Error(L"Unexpected ).", t.pos);
			}
		}

		// When we get here, we will either get a comma, meaning another initializer in the list, or a
		// {, meaning we're done. We need to make sure not to consume the {, so we don't really check
		// for it and leave that to the caller.
	} while (tok.skipIf(L","));
}

// Parse a variable or function.
static void parseMember(Tokenizer &tok, ParseEnv &env, Namespace &addTo, Access access, bool isStatic) {
	if (!tok.more() || tok.skipIf(L";"))
		return;

	// Primitive parsing of annotations (useful with the [[deprecated]] annotation).
	if (tok.skipIf(L"[")) {
		tok.expect(L"[");
		tok.skip();
		tok.expect(L"]");
		tok.expect(L"]");
	}

	Token name(L"", SrcPos());
	bool castFn = tok.skipIf(L"STORM_CAST_CTOR");
	bool exportFn = castFn || tok.skipIf(L"STORM_CTOR");

	bool isVirtual = tok.skipIf(L"virtual");
	tok.skipIf(L"inline"); // The combination 'virtual inline' is insane, but we'll roll with it...

	Auto<Doc> doc = getDoc(tok, env);

	// First, we should have a type.
	Auto<TypeRef> type;
	if (tok.skipIf(L"~")) {
		type = new NamedType(tok.peek().pos, L"void");
		tok.skip(); // Ignore the name of the destructor.
		name.token = Function::dtor;
	} else {
		type = parseTypeRef(tok);
	}

	// Something else, skip.
	if (!tok.more() || tok.skipIf(L";"))
		return;

	// Is this function supposed to be exported to Storm?
	exportFn |= tok.skipIf(L"STORM_FN");
	bool assignFn = tok.skipIf(L"STORM_ASSIGN");
	exportFn |= assignFn;

	// Other markers...
	tok.skipIf(L"CODECALL");

	bool isCtor = false;

	name.pos = type->pos;
	String stormName;
	if (tok.peek().token != L"(") {
		// Then, we should have the member's name.
		name = tok.next();
		if (name.token == L"operator") {
			// < and > are tokenized one at a time.
			if (tok.skipIf(L"<")) {
				if (tok.skipIf(L"<"))
					name.token += L" <<";
				else
					name.token += L" <";
			} else if (tok.skipIf(L">")) {
				if (tok.skipIf(L">"))
					name.token += L" >>";
				else
					name.token += L" >";
			} else if (tok.skipIf(L"[")) {
				if (tok.skipIf(L"]"))
					name.token += L" []";
				else
					name.token += L" [";
			} else {
				name.token += L" " + tok.next().token;
			}
			// Is this a ?= operator?
			if (tok.skipIf(L"="))
				name.token += L"=";
		} else if (name.token == L"STORM_NAME") {
			tok.expect(L"(");
			name = tok.next();
			tok.expect(L",");
			stormName = tok.next().token;
			tok.expect(L")");
		}
	} else if (name.token != Function::dtor) {
		// Constructor
		isCtor = true;
		type = new NamedType(name.pos, L"void");
		name.token = Function::ctor;
	}

	if (tok.skipIf(L"(")) {
		// Function.
		Function f(CppName(name.token), env.pkg, access, name.pos, doc, type);
		if (!stormName.empty())
			f.stormName = stormName;
		f.set(Function::isVirtual, isVirtual);
		f.set(Function::isAssign, assignFn);
		f.set(Function::castMember, castFn);

		if (!tok.skipIf(L")")) {
			f.params.push_back(parseTypeRef(tok));
			f.paramNames.push_back(tok.next().token);
			skipArray(tok, !exportFn);

			while (tok.more() && tok.skipIf(L",")) {
				if (tok.skipIf(L"...")) {
					if (exportFn)
						throw Error(L"Can not export variadic functions.", name.pos);

					// No more parameters are allowed after a ...
					break;
				}
				f.params.push_back(parseTypeRef(tok));
				f.paramNames.push_back(tok.next().token);
				skipArray(tok, !exportFn);
			}

			tok.expect(L")");
		}

		if (tok.skipIf(L"const"))
			f.set(Function::isConst);

		while (true) {
			Token next = tok.peek();
			if (next.token == L";" || next.token == L"{")
				break;

			if (tok.skipIf(L"ON")) {
				tok.expect(L"(");
				f.thread = parseName(tok);
				tok.expect(L")");
			} else if (tok.skipIf(L"ABSTRACT")) {
				f.set(Function::isAbstract);
			} else if (tok.skipIf(L"override")) {
				// We don't really handle this, but why not allow it?
				// TODO: Perhaps we want to set 'isVirtual'?
			} else if (tok.skipIf(L"final")) {
				// This means 'final'.
				f.clear(Function::isVirtual);
			} else if (isCtor && tok.skipIf(L":")) {
				skipInitList(tok);
			} else {
				throw Error(L"Unsupported function modifier: " + tok.peek().token, tok.peek().pos);
			}
		}

		f.set(Function::isStatic, isStatic);
		f.set(Function::exported, env.exportAll);

		// Save if 'exportFn' is there.
		if (exportFn || f.name == Function::dtor)
			// We need to export abstract functions so that we can generate stubs for them!
			if (env.exportAll || f.has(Function::isAbstract))
				addTo.add(f);

	} else if (tok.skipIf(L"[")) {
		// We're only interested when we're not dealing with a non-static array.
		if (!isStatic)
			throw Error(L"C-style arrays not supported in classes exposed to Storm.", name.pos);
	} else if (tok.skipIf(L"=")) {
		// Initialized variable. Skip the initialization part.
		while (!tok.skipIf(L";"))
			tok.skip();

		// We don't do static variables.
		if (!isStatic) {
			Variable v(CppName(name.token), type, access, name.pos, doc);
			if (!stormName.empty())
				v.stormName = stormName;
			addTo.add(v);
		}
	} else if (tok.skipIf(L";")) {
		// We don't do static variables.
		if (!isStatic) {
			// Variable.
			Variable v(CppName(name.token), type, access, name.pos, doc);
			if (!stormName.empty())
				v.stormName = stormName;
			addTo.add(v);
		}
	} else {
		// Unknown construct; ignore it.
	}
}

// Parse an enum declaration.
static void parseEnum(Tokenizer &tok, ParseEnv &env, const CppName &inside) {
	Token name = tok.next();
	if (name.token == L"STORM_HIDDEN" && tok.skipIf(L"(")) {
		// This enum is supposed to be hidden from Storm.
		tok.skip();
		tok.expect(L")");
		name.token = L"";
	}

	// Forward-declaration?
	if (tok.skipIf(L";"))
		return;

	Auto<Doc> doc = getDoc(tok, env);

	if (name.token == L"{") {
		name.token = L"";
	} else {
		tok.expect(L"{");
	}

	// Add the type.
	CppName fullName = inside + name.token;
	Auto<Enum> type = new Enum(fullName, env.pkg, name.pos, doc);

	ParseEnv childEnv = env;
	childEnv.doc = Auto<Doc>();
	do {
		Token member = tok.next();
		if (member.token == L"}")
			break;

		Token stormName = member;
		if (member.token == L"STORM_NAME") {
			tok.expect(L"(");
			member = tok.next();
			tok.expect(L",");
			stormName = tok.next();
			tok.expect(L")");
		}

		type->members.push_back(member.token);
		type->stormMembers.push_back(stormName.token);
		type->memberDoc.push_back(getDoc(tok, childEnv));

		if (tok.skipIf(L"=")) {
			// Some expression for initialization. Skip it.
			while (tok.peek().token != L"," && tok.peek().token != L"}")
				tok.next();
		}

	} while (tok.skipIf(L","));
	tok.clearComment();
	tok.skipIf(L"}");
	tok.expect(L";");

	type->external = !env.exportAll;
	if (!name.token.empty())
		env.world.add(type);
}

// Parse the parent class of a type.
static Auto<Class> parseParent(Tokenizer &tok, ParseEnv &env, const CppName &fullName, const String &pkg, const SrcPos &pos) {
	Auto<Doc> doc = getDoc(tok, env);
	Auto<Class> result = new Class(fullName, pkg, pos, doc);
	if (!tok.skipIf(L":"))
		return result;

	if (tok.skipIf(L"public")) {
		if (tok.skipIf(L"STORM_HIDDEN")) {
			tok.expect(L"(");
			result->parent = parseName(tok);
			result->set(Class::hiddenParent);
			tok.expect(L")");
		} else if (tok.skipIf(L"ObjectOn")) {
			tok.expect(L"<");
			result->parent = CppName(L"storm::TObject");
			result->clear(Class::hiddenParent);
			result->thread = parseName(tok);
			tok.expect(L">");
		} else {
			result->parent = parseName(tok);
			result->clear(Class::hiddenParent);
		}
	}

	// Skip any multi-inheritance or private inheritance.
	while (tok.peek().token != L"{")
		tok.skip();

	return result;
}

// Parse a type.
static void parseType(Tokenizer &tok, ParseEnv &env, const CppName &inside, bool isPrivate) {
	// Exported exception?
	bool exported = tok.skipIf(L"EXCEPTION_EXPORT");

	Token name = tok.next();

	// Forward-declaration?
	if (tok.skipIf(L";"))
		return;

	CppName fullName = inside + name.token;
	Auto<Class> type = parseParent(tok, env, fullName, env.pkg, name.pos);

	tok.expect(L"{");

	// Are we interested in this class at all?
	if (tok.skipIf(L"STORM_CLASS")) {
		type->clear(Class::value);
	} else if (tok.skipIf(L"STORM_ABSTRACT_CLASS")) {
		type->clear(Class::value);
		type->set(Class::abstract);
	} else if (tok.skipIf(L"STORM_VALUE")) {
		type->set(Class::value);
	} else if (tok.skipIf(L"STORM_EXCEPTION")) {
		type->clear(Class::value);
		type->set(Class::exception);
	} else if (tok.skipIf(L"STORM_EXCEPTION_BASE")) {
		type->clear(Class::value);
		type->set(Class::rootException);
	} else if (exported) {
		// Warn about STORM_EXCEPTION not being present.
		throw Error(L"Exceptions from Storm must be exported with STORM_EXCEPTION as well as EXCEPTION_EXPORT.", tok.peek().pos);
	} else {
		// No. Ignore the rest of the block!
		parseBlock(tok);
		return;
	}

	tok.expect(L";");

	if (type->has(Class::exception) && !exported)
		throw Error(L"Exceptions must be exported with EXCEPTION_EXPORT after 'class'.", name.pos);

	Access access = aPrivate;
	bool isStatic = false;
	while (tok.more()) {
		Token t = tok.peek();
		bool wasStatic = isStatic;
		isStatic = false;

		if (t.token == L"{") {
			tok.skip();
			parseBlock(tok);
		} else if (t.token == L"}") {
			tok.skip();
			break;
		} else if (t.token == L"class" || t.token == L"struct") {
			tok.skip();
			skipAlignAs(tok);
			ParseEnv sub = env;
			if (sub.pkg.empty())
				sub.pkg = name.token;
			else
				sub.pkg += L"." + name.token;
			parseType(tok, sub, fullName, access == aPrivate);
		} else if (t.token == L"extern") {
			tok.skip();
		} else if (t.token == L"enum") {
			tok.skip();
			ParseEnv sub = env;
			if (sub.pkg.empty())
				sub.pkg = name.token;
			else
				sub.pkg += L"." + name.token;
			parseEnum(tok, sub, fullName);
		} else if (t.token == L"template") {
			// Two possibilities:
			while (true) {
				// Either, we have something that ends with a semicolon. Then we're done.
				if (tok.skipIf(L";")) {
					break;
				}
				// Or, we have something declared inline. In that case, we skip the body as well.
				if (tok.skipIf(L"{")) {
					parseBlock(tok);
					break;
				}

				tok.skip();
			}
		} else if (t.token == L"public") {
			tok.skip();
			tok.expect(L":");
			access = aPublic;
		} else if (t.token == L"private") {
			tok.skip();
			tok.expect(L":");
			access = aPrivate;
		} else if (t.token == L"protected") {
			tok.skip();
			tok.expect(L":");
			access = aProtected;
		} else if (t.token == L"static") {
			tok.skip();
			isStatic = true;
		} else if (t.token == L"friend") {
			while (!tok.skipIf(L";"))
				tok.skip();
		} else if (t.token == L"using" || t.token == L"typedef") {
			while (!tok.skipIf(L";"))
				tok.skip();
		} else if (t.token == L"inline") {
			tok.skip();
		} else {
			ClassNamespace ns(env.world, *type);
			parseMember(tok, env, ns, access, wasStatic);
		}
	}

	type->external = !env.exportAll;
	type->isPrivate = isPrivate;
	env.world.add(type);

	tok.clearComment();
	tok.expect(L";");
}

// Parse in the context of a namespace.
static void parseNamespace(Tokenizer &tok, ParseEnv &env, const CppName &name) {
	while (tok.more()) {
		Token t = tok.peek();

		if (t.token == L"class" || t.token == L"struct") {
			tok.skip();
			skipAlignAs(tok);
			parseType(tok, env, name, false);
		} else if (t.token == L"enum") {
			tok.skip();
			parseEnum(tok, env, name);
		} else if (t.token == L"template") {
			// Skip until we find a {, and skip the body as well.
			while (!tok.skipIf(L"{"))
				tok.skip();
			parseBlock(tok);
		} else if (t.token == L"extern") {
			tok.skip();
			tok.skipIf(L"\"C\""); // for 'extern "C" {}'
		} else if (t.token == L"static") {
			tok.skip();
		} else if (t.token == L"using") {
			tok.next();
			if (tok.skipIf(L"namespace")) {
				// Skip it.
				while (!tok.skipIf(L";"))
					tok.skip();
			} else {
				// Add an alias.
				CppName a = parseName(tok);
				env.world.aliases.insert(make_pair(name + a.last(), a));
				tok.expect(L";");
			}
		} else if (t.token == L"typedef") {
			while (!tok.skipIf(L";"))
				tok.skip();
		} else if (t.token == L"STORM_PKG") {
			tok.next();
			tok.expect(L"(");
			// TODO: Take care of this name properly!
			String pkg = tok.next().token;
			while (tok.skipIf(L"."))
				pkg = pkg + L"." + tok.next().token;
			tok.expect(L")");
			tok.expect(L";");
			env.pkg = pkg;
		} else if (t.token == L"namespace") {
			tok.skip();
			Token n = tok.next();
			if (n == L"{") {
				// Anonymous namespace.
				parseNamespace(tok, env, name);
			} else {
				tok.expect(L"{");
				parseNamespace(tok, env, name + n.token);
			}
		} else if (t.token == L"BITMASK_OPERATORS") {
			tok.skip();
			tok.expect(L"(");
			SrcPos pos = tok.peek().pos;
			CppName n = parseName(tok);
			Type *t = env.world.types.find(n, name, pos);
			if (Enum *e = as<Enum>(t)) {
				e->bitmask = true;
			} else {
				throw Error(L"The type " + toS(n) + L" is not an enum.", pos);
			}
			tok.expect(L")");
			tok.expect(L";");
		} else if (t.token == L"STORM_TEMPLATE") {
			tok.skip();
			tok.expect(L"(");
			Token tName = tok.next();
			tok.expect(L",");
			CppName gen = parseName(tok);
			Auto<Doc> doc = getDoc(tok, env);
			Auto<Template> t = new Template(name + tName.token, env.pkg, gen, tName.pos, doc);
			t->external = !env.exportAll;
			env.world.templates.insert(t);
			tok.expect(L")");
			tok.expect(L";");
		} else if (t.token == L"STORM_PRIMITIVE") {
			tok.skip();
			tok.expect(L"(");
			Token tName = tok.next();
			tok.expect(L",");
			Auto<Doc> doc = getDoc(tok, env);
			CppName gen = parseName(tok);
			Auto<Primitive> p = new Primitive(name + tName.token, env.pkg, gen, tName.pos, doc);
			p->external = !env.exportAll;
			env.world.types.insert(p);
			tok.expect(L")");
			tok.expect(L";");
		} else if (t.token == L"STORM_UNKNOWN_PRIMITIVE") {
			tok.skip();
			tok.expect(L"(");
			Token tName = tok.next();
			tok.expect(L",");
			CppName gen = parseName(tok);
			Auto<UnknownPrimitive> p = new UnknownPrimitive(name + tName.token, env.pkg, gen, tName.pos);
			p->external = !env.exportAll;
			env.world.types.insert(p);
			tok.expect(L")");
			tok.expect(L";");
		} else if (t.token == L"STORM_THREAD") {
			tok.skip();
			tok.expect(L"(");
			Token tName = tok.next();
			Auto<Doc> doc = getDoc(tok, env);
			Auto<Thread> t = new Thread(name + tName.token, env.pkg, tName.pos, doc, !env.exportAll);
			env.world.threads.insert(t);
			tok.expect(L")");
			tok.expect(L";");
		} else if (t.token.startsWith(L"PROXY")) {
			tok.skip();
			tok.expect(L"(");
			while (!tok.skipIf(L")"))
				tok.skip();
			tok.expect(L";");
		} else if (t.token == L"{") {
			tok.skip();
			parseBlock(tok);
		} else if (t.token == L"}") {
			tok.clearComment();
			tok.skip();
			break;
		} else if (t.token == L";") {
			// Stray semicolon, skip it.
			tok.skip();
		} else {
			// Everything is considered 'public' and non-static outside of a class.
			WorldNamespace tmp(env.world, name);
			parseMember(tok, env, tmp, aPublic, false);
		}
	}
}

// Parse an entire header file.
static void parseFile(nat id, World &world) {
	Tokenizer tok(id);
	ParseEnv env = { L"", id >= SrcPos::firstExport, world };
	parseNamespace(tok, env, CppName());
}

// Parse a license file.
static void parseLicense(const Path &path, World &world) {
	TextReader *src = TextReader::create(new FileStream(path, Stream::mRead));

	String cond = src->getLine();
	String pkg;
	if (cond.empty() || cond[0] != '#') {
		pkg = cond;
		cond = L"";
	} else {
		cond = cond.substr(1);
		pkg = src->getLine();
	}
	String title = src->getLine();
	String author = src->getLine();
	String body = src->getAll();

	delete src;

	world.licenses.push_back(License(path.titleNoExt(), pkg, cond, title, author, body));
}

// Parse a version file.
static void parseVersion(const Path &path, World &world) {
	TextReader *src = TextReader::create(new FileStream(path, Stream::mRead));

	String pkg = src->getLine();
	String ver = src->getLine();

	delete src;

	world.versions.push_back(Version(path.titleNoExt(), pkg, ver));
}

void parseWorld(World &world, const vector<Path> &licenses, const vector<Path> &versions) {
	for (nat i = 0; i < SrcPos::files.size(); i++) {
		parseFile(i, world);
	}
	for (nat i = 0; i < licenses.size(); i++) {
		parseLicense(licenses[i], world);
	}
	for (nat i = 0; i < versions.size(); i++) {
		parseVersion(versions[i], world);
	}
}
