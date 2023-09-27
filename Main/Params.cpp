#include "stdafx.h"
#include "Params.h"

struct StatePtr;
typedef StatePtr (*State)(const wchar_t *, Params &);

struct StatePtr {
	StatePtr() : p(null) {}
    StatePtr(State p) : p(p) {}
	StatePtr operator () (const wchar_t *s, Params &l) { return (*p)(s, l); }
	operator bool() { return p != null; }
    State p;
};

#define PARSE_ERROR(msg)						\
	result.mode = Params::modeError;			\
	result.modeParam = msg;						\
	result.modeParam2 = null;					\
	return StatePtr()

#define EXPECT_MORE(msg)						\
	if (arg == null) {							\
		PARSE_ERROR(msg);						\
	}

static StatePtr start(const wchar_t *arg, Params &result);

static StatePtr function(const wchar_t *arg, Params &result) {
	EXPECT_MORE(L"Missing function name.");
	result.modeParam = arg;
	return &start;
}

static StatePtr tests(const wchar_t *arg, Params &result) {
	EXPECT_MORE(L"Missing package name.");
	result.modeParam = arg;
	return &start;
}

static StatePtr replCommand(const wchar_t *arg, Params &result) {
	EXPECT_MORE(L"Missing repl command.");
	result.modeParam2 = arg;
	return &start;
}

static StatePtr root(const wchar_t *arg, Params &result) {
	EXPECT_MORE(L"Missing root path.");
	if (result.root) {
		PARSE_ERROR(L"Root already set once!");
	}
	result.root = arg;
	return &start;
}

static StatePtr importShort(const wchar_t *arg, Params &result) {
	EXPECT_MORE(L"Missing import path.");
	Import i = {
		arg,
		null,
		false
	};
	result.import.push_back(i);
	return &start;
}

static StatePtr importPath(const wchar_t *arg, Params &result) {
	EXPECT_MORE(L"Missing import path.");
	result.import.back().path = arg;
	return &start;
}

static StatePtr importLong(const wchar_t *arg, Params &result) {
	EXPECT_MORE(L"Missing import target.");
	Import i = {
		null,
		arg,
		false
	};
	result.import.push_back(i);
	return &importPath;
}

static StatePtr replLang(const wchar_t *arg, Params &result) {
	EXPECT_MORE(L"Missing language for the REPL.");
	result.mode = Params::modeRepl;
	result.modeParam = arg;
	return &start;
}

static StatePtr consumeArgv(const wchar_t *arg, Params &result) {
	if (arg == null)
		return StatePtr();

	result.argv.push_back(arg);
	return &consumeArgv;
}

static StatePtr start(const wchar_t *arg, Params &result) {
	if (arg == null) {
		return StatePtr();
	} else if (wcscmp(arg, L"-?") == 0 || wcscmp(arg, L"--help") == 0) {
		result.mode = Params::modeHelp;
		return StatePtr();
	} else if (wcscmp(arg, L"-f") == 0) {
		result.mode = Params::modeFunction;
		return &function;
	} else if (wcscmp(arg, L"-t") == 0) {
		result.mode = Params::modeTests;
		return &tests;
	} else if (wcscmp(arg, L"-T") == 0) {
		result.mode = Params::modeTestsRec;
		return &tests;
	} else if (wcscmp(arg, L"-c") == 0) {
		result.mode = Params::modeRepl;
		return &replCommand;
	} else if (wcscmp(arg, L"-i") == 0) {
		return &importShort;
	} else if (wcscmp(arg, L"-I") == 0) {
		return &importLong;
	} else if (wcscmp(arg, L"-r") == 0) {
		return &root;
	} else if (wcscmp(arg, L"-l") == 0) {
		return &replLang;
	} else if (wcscmp(arg, L"--version") == 0) {
		result.mode = Params::modeVersion;
		return StatePtr();
	} else if (wcscmp(arg, L"--server") == 0) {
		result.mode = Params::modeServer;
		return StatePtr();
	} else if (wcscmp(arg, L"--") == 0) {
		return &consumeArgv;
	} else {
		// This is the same as -i
		Import i = {
			arg,
			null,
			true
		};
		result.import.push_back(i);
		return &start;
	}
}

Params::Params(int argc, const wchar_t *argv[])
	: mode(modeAuto),
	  root(null),
	  modeParam(L"bs"),
	  modeParam2(null),
	  import() {

	StatePtr state = &start;

	for (int i = 1; i < argc; i++) {
		if (state) {
			state = state(argv[i], *this);
		} else if (mode != Params::modeError) {
			mode = Params::modeError;
			modeParam = L"Unknown parameter: ";
			modeParam2 = argv[i];
			break;
		} else {
			break;
		}
	}

	if (state && mode != Params::modeError)
		state(null, *this);
}

void help(const wchar_t *cmd) {
	wcout << L"Usage: " << endl;
	wcout << cmd << L"                  - launch the default REPL." << endl;
	wcout << cmd << L" <path>           - import file or directory (like -i), launch function 'main' if able." << endl;
	wcout << cmd << L" -l <language>    - launch the REPL for <language>." << endl;
	wcout << cmd << L" -f <function>    - run <function> then exit." << endl;
	wcout << cmd << L" -t <package>     - run all tests in <package> then exit." << endl;
	wcout << cmd << L" -T <package>     - run all tests in <package> and all sub-packages then exit." << endl;
	wcout << cmd << L" -i <path>        - import directory or file as the name of the package." << endl;
	wcout << cmd << L" -I <name> <path> - import package (directory) at <path> as <name>." << endl;
	wcout << cmd << L" -c <expr>        - evaluate <expr> in the default REPL." << endl;
	wcout << cmd << L" -r <path>        - use <path> as the root path." << endl;
	wcout << cmd << L" --version        - print the current version and exit." << endl;
	wcout << cmd << L" --server         - start the language server." << endl;
	wcout << cmd << L" -- <arguments>   - interpret remaining arguments as parameters to the program." << endl;
}
