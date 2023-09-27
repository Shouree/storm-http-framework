#include "stdafx.h"
#include "Params.h"
#include "Debug.h"
#include "Compiler/Server/Main.h"
#include "Compiler/Exception.h"
#include "Compiler/Engine.h"
#include "Compiler/Package.h"
#include "Compiler/Repl.h"
#include "Compiler/Version.h"
#include "Core/Timing.h"
#include "Core/Io/StdStream.h"
#include "Core/Io/Text.h"

void runRepl(Engine &e, const wchar_t *lang, Repl *repl) {
	TextInput *input = e.stdIn();
	TextOutput *output = e.stdOut();

	Str *line = null;
	while (!repl->exit()) {
		{
			StrBuf *prompt = new (e) StrBuf();
			if (line)
				*prompt << L"? ";
			else
				*prompt << lang << L"> ";

			output->write(prompt->toS());
			output->flush();
		}

		Str *data = input->readLine();
		if (!line) {
			line = data;
		} else {
			StrBuf *to = new (repl) StrBuf();
			*to << line << L"\n" << data;
			line = to->toS();
		}

		try {
			if (repl->eval(line))
				line = null;
		} catch (const storm::Exception *err) {
			output->writeLine(err->toS());
			line = null;
		} catch (const ::Exception &err) {
			std::wostringstream t;
			t << err << endl;
			output->writeLine(new (e) Str(t.str().c_str()));
			line = null;
		}
	}
}

int runRepl(Engine &e, Duration bootTime, const Path &root, const wchar_t *lang, const wchar_t *input) {
	if (!input) {
		wcout << L"Welcome to the Storm compiler!" << endl;
		wcout << L"Root directory: " << root << endl;
		wcout << L"Compiler boot in " << bootTime << endl;
	}

	if (!lang)
		lang = L"bs";

	Name *replName = new (e) Name();
	replName->add(new (e) Str(S("lang")));
	replName->add(new (e) Str(lang));
	replName->add(new (e) Str(S("repl")));
	Function *replFn = as<Function>(e.scope().find(replName));
	if (!replFn) {
		wcerr << L"Could not find a repl for " << lang << L": no function named " << replName << L" exists." << endl;
		return 1;
	}

	Value replVal(Repl::stormType(e));
	if (!replVal.mayReferTo(replFn->result)) {
		wcerr << L"The function " << replName << L" must return a subtype of "
			  << replVal << L", not " << replFn->result << endl;
		return 1;
	}

	typedef Repl *(*CreateRepl)();
	CreateRepl createRepl = (CreateRepl)replFn->ref().address();
	Repl *repl = (*createRepl)();

	if (input) {
		if (!repl->eval(new (e) Str(input))) {
			wcerr << L"The input given to the REPL does not represent a complete input. Try again!" << endl;
			return 1;
		}
	} else {
		repl->greet();
		runRepl(e, lang, repl);
	}

	return 0;
}

bool hasColon(const wchar_t *function) {
	for (; *function; function++)
		if (*function == ':')
			return true;
	return false;
}

int runFunction(Engine &e, Function *fn) {
	Value r = fn->result;
	if (!r.returnInReg()) {
		wcout << L"Found a main function: " << fn->identifier() << L", but it returns a value-type." << endl;
		wcout << L"This is not currently supported. Try running it through the REPL instead." << endl;
		return 1;
	}

	// We can ignore the return value. But if it is an integer, we capture it and return it.
	code::Primitive resultType;
	if (code::PrimitiveDesc *resultDesc = as<code::PrimitiveDesc>(r.desc(e)))
		resultType = resultDesc->v;

	Nat resultSize = 0;
	if (resultType.kind() == code::primitive::integer)
		resultSize = resultType.size().current();

	RunOn run = fn->runOn();
	const void *addr = fn->ref().address();
	if (run.state == RunOn::named) {
		if (resultSize == 1) {
			os::FnCall<Byte, 1> call = os::fnCall();
			os::Future<Byte> result;
			os::Thread on = run.thread->thread()->thread();
			os::UThread::spawn(addr, false, call, result, &on);
			return result.result();
		} else if (resultSize == 4) {
			os::FnCall<Int, 1> call = os::fnCall();
			os::Future<Int> result;
			os::Thread on = run.thread->thread()->thread();
			os::UThread::spawn(addr, false, call, result, &on);
			return result.result();
		} else if (resultSize == 8) {
			os::FnCall<Long, 1> call = os::fnCall();
			os::Future<Long> result;
			os::Thread on = run.thread->thread()->thread();
			os::UThread::spawn(addr, false, call, result, &on);
			return (int)result.result();
		} else {
			os::FnCall<void, 1> call = os::fnCall();
			os::Future<void> result;
			os::Thread on = run.thread->thread()->thread();
			os::UThread::spawn(addr, false, call, result, &on);
			result.result();
		}
	} else {
		if (resultSize == 1) {
			typedef Byte (*Fn1)();
			Fn1 p = (Fn1)addr;
			return (*p)();
		} else if (resultSize == 4) {
			typedef Int (*Fn4)();
			Fn4 p = (Fn4)addr;
			return (*p)();
		} else if (resultSize == 8) {
			typedef Long (*Fn8)();
			Fn8 p = (Fn8)addr;
			return (int)(*p)();
		} else {
			typedef void (*Fn)();
			Fn p = (Fn)addr;
			(*p)();
		}
	}
	return 0;
}

int runFunction(Engine &e, const wchar_t *function) {
	SimpleName *name = parseSimpleName(new (e) Str(function));
	Named *found = e.scope().find(name);
	if (!found) {
		wcerr << L"Could not find " << function << endl;
		if (hasColon(function))
			wcerr << L"Did you mean to use '.' instead of ':'?" << endl;
		return 1;
	}

	Function *fn = as<Function>(found);
	if (!fn) {
		wcout << function << L" is not a function." << endl;
		return 1;
	}

	return runFunction(e, fn);
}

bool tryRun(Engine &e, Array<Package *> *pkgs, int &result) {
	SimplePart *mainPart = new (e) SimplePart(new (e) Str(S("main")));

	for (Nat i = 0; i < pkgs->count(); i++) {
		Package *pkg = pkgs->at(i);
		Function *main = as<Function>(pkg->find(mainPart, Scope(pkg)));
		if (!main)
			continue;

		result = runFunction(e, main);
		return true;
	}

	// Failed.
	return false;
}

int runTests(Engine &e, const wchar_t *package, bool recursive) {
	SimpleName *name = parseSimpleName(new (e) Str(package));
	Named *found = e.scope().find(name);
	if (!found) {
		wcerr << L"Could not find " << package << endl;
		return 1;
	}

	Package *pkg = as<Package>(found);
	if (!pkg) {
		wcerr << package << L" is not a package." << endl;
		return 1;
	}

	SimpleName *testMain = parseSimpleName(e, S("test.runCmdline"));
	testMain->last()->params->push(Value(StormInfo<Package>::type(e)));
	testMain->last()->params->push(Value(StormInfo<Bool>::type(e)));
	Function *f = as<Function>(e.scope().find(testMain));
	if (!f) {
		wcerr << L"Could not find the test implementation's main function: " << testMain << endl;
		return 1;
	}

	if (f->result.type != StormInfo<Bool>::type(e)) {
		wcerr << L"The signature of the test implementation's main function ("
			  << testMain << L") is incorrect. Expected Bool, but got "
			  << f->result.type->identifier() << endl;
		return 1;
	}

	os::FnCall<Bool> c = os::fnCall().add(pkg).add(recursive);
	Bool ok = c.call(f->ref().address(), false);
	return ok ? 0 : 1;
}

// Returns array of paths that we should try to run if they exist.
Array<Package *> *importPkgs(Engine &into, const Params &p) {
	Array<Package *> *result = new (into) Array<Package *>();

	for (Nat i = 0; i < p.import.size(); i++) {
		const Import &import = p.import[i];

		Url *path = parsePath(new (into) Str(import.path));
		if (!path->absolute())
			path = cwdUrl(into)->push(path);

		// Update file/directory flag.
		path = path->updated();

		if (!path->exists()) {
			wcerr << L"WARNING: The path " << *path << L" could not be found. Will not import it." << endl;
			continue;
		}

		SimpleName *n = null;
		if (import.into) {
			n = parseSimpleName(new (into) Str(import.into));
		} else {
			n = parseSimpleName(path->title());
		}

		if (n->empty())
			continue;

		NameSet *ns = into.nameSet(n->parent(), true);
		Package *pkg = new (into) Package(n->last()->name, path);
		ns->add(pkg);

		if (import.tryRun)
			result->push(pkg);
	}

	return result;
}

void showVersion(Engine &e) {
	Array<VersionTag *> *tags = storm::versions(e.package(S("core")));
	if (tags->empty()) {
		wcerr << L"Error: No version information found. Make sure this release was compiled correctly." << std::endl;
		return;
	}

	VersionTag *best = tags->at(0);
	for (Nat i = 1; i < tags->count(); i++) {
		if (best->path()->count() > tags->at(i)->path()->count())
			best = tags->at(i);
	}

	wcout << best->version << std::endl;
}

void createArgv(Engine &e, const vector<const wchar_t *> &argv) {
	Array<Str *> *out = new (e) Array<Str *>();
	out->reserve(argv.size());
	for (size_t i = 0; i < argv.size(); i++) {
		*out << new (e) Str(argv[i]);
	}
	e.argv(out);
}

int stormMain(int argc, const wchar_t *argv[]) {
	Params p(argc, argv);

	switch (p.mode) {
	case Params::modeHelp:
		help(argv[0]);
		return 0;
	case Params::modeError:
		wcerr << L"Error in parameters: " << p.modeParam;
		if (p.modeParam2)
			wcerr << p.modeParam2;
		wcerr << endl;
		return 1;
	}

	Moment start;

	// Start an Engine. TODO: Do not depend on 'Path'.
#if defined(DEBUG)
	Path root = Path::dbgRoot() + L"root";
#elif defined(STORM_ROOT)
	Path root = Path(STRING(STORM_ROOT));
#else
	Path root = Path::executable() + L"root";
#endif
	if (p.root) {
		root = Path(String(p.root));
		if (!root.isAbsolute())
			root = Path::cwd() + root;
	}

	Engine e(root, Engine::reuseMain, &argv);
	Moment end;

	if (!p.argv.empty())
		createArgv(e, p.argv);

	Array<Package *> *runPkgs = importPkgs(e, p);

	int result = 1;

	try {
		switch (p.mode) {
		case Params::modeAuto:
			if (!tryRun(e, runPkgs, result)) {
				// If none of them contained a main function, launch the repl.
				result = runRepl(e, (end - start), root, null, null);
			}
			break;
		case Params::modeRepl:
			result = runRepl(e, (end - start), root, p.modeParam, p.modeParam2);
			break;
		case Params::modeFunction:
			result = runFunction(e, p.modeParam);
			break;
		case Params::modeTests:
			result = runTests(e, p.modeParam, false);
			break;
		case Params::modeTestsRec:
			result = runTests(e, p.modeParam, true);
			break;
		case Params::modeVersion:
			showVersion(e);
			result = 0;
			break;
		case Params::modeServer:
			server::run(e, proc::in(e), proc::out(e));
			result = 0;
			break;
		default:
			throw new (e) InternalError(S("Unknown mode."));
		}
	} catch (const storm::Exception *e) {
		wcerr << e << endl;
		result = 1;
		// Fall-thru to wait for UThreads.
	} catch (const ::Exception &e) {
		// Sometimes, we need to print the exception before the engine is destroyed.
		wcerr << e << endl;
		return 1;
	}

	// Allow 1 s for all UThreads on the Compiler thread to terminate.
	Moment waitStart;
	while (os::UThread::leave() && Moment() - waitStart > time::s(1))
		;

	return result;
}

#ifdef WINDOWS

int _tmain(int argc, const wchar *argv[]) {
	try {
		return stormMain(argc, argv);
	} catch (const ::Exception &e) {
		wcerr << L"Unhandled exception: " << e << endl;
		return 1;
	}
}

#else

int main(int argc, const char *argv[]) {
	try {
		vector<String> args(argv, argv+argc);
		vector<const wchar_t *> c_args(argc);
		for (int i = 0; i < argc; i++)
			c_args[i] = args[i].c_str();

		return stormMain(argc, &c_args[0]);
	} catch (const ::Exception &e) {
		wcerr << "Unhandled exception: " << e << endl;
		return 1;
	}
}

#endif
