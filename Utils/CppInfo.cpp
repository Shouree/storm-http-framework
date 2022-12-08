#include "stdafx.h"
#include "CppInfo.h"
#include "Path.h"
#include "DbgHelper.h"

bool CppInfo::translate(void *ip, void *&fn, int &offset) const {
	// We don't know very much... We do, however, know that we're last. So we can always return 'true'.
	fn = ip;
	offset = 0;
	return true;
}

#if defined(WINDOWS)

// Easy creation of a SYMBOL_INFO
struct SymInfo : SYMBOL_INFO {
	SymInfo() {
		SizeOfStruct = sizeof(SYMBOL_INFO);
		MaxNameLen = maxSize;
	}

	enum { maxSize = 512 };
	// Rest of the name...
	wchar data[maxSize];
};

// Found somewhere, from CVCONST.H in DIA SDK (sic)
enum BasicType {
	btNoType = 0,
	btVoid = 1,
	btChar = 2,
	btWChar = 3,
	btInt = 6,
	btUInt = 7,
	btFloat = 8,
	btBCD = 9,
	btBool = 10,
	btEnum = 12, // Mine
	btLong = 13,
	btULong = 14,
	btCurrency = 25,
	btDate = 26,
	btVariant = 27,
	btComplex = 28,
	btBit = 29,
	btBSTR = 30,
	btHresult = 31
};

// From MSDN.
enum SymTagEnum {
	SymTagNull,
	SymTagExe,
	SymTagCompiland,
	SymTagCompilandDetails,
	SymTagCompilandEnv,
	SymTagFunction,
	SymTagBlock,
	SymTagData,
	SymTagAnnotation,
	SymTagLabel,
	SymTagPublicSymbol,
	SymTagUDT,
	SymTagEnum,
	SymTagFunctionType,
	SymTagPointerType,
	SymTagArrayType,
	SymTagBaseType,
	SymTagTypedef,
	SymTagBaseClass,
	SymTagFriend,
	SymTagFunctionArgType,
	SymTagFuncDebugStart,
	SymTagFuncDebugEnd,
	SymTagUsingNamespace,
	SymTagVTableShape,
	SymTagVTable,
	SymTagCustom,
	SymTagThunk,
	SymTagCustomType,
	SymTagManagedType,
	SymTagDimension,
	SymTagCallSite,
	SymTagInlineSite,
	SymTagBaseInterface,
	SymTagVectorType,
	SymTagMatrixType,
	SymTagHLSLType
};

static bool baseType(GenericOutput &to, DWORD index) {
	switch (index) {
	case btVoid:
		to.put(L"void");
		return true;
	case btChar:
		to.put(L"char");
		return true;
	case btWChar:
		to.put(L"wchar_t");
		return true;
	case btInt:
		to.put(L"int");
		return true;
	case btUInt:
		to.put(L"nat");
		return true;
	case btFloat:
		to.put(L"float");
		return true;
	case btBCD:
		to.put(L"BCD");
		return true;
	case btBool:
		to.put(L"bool");
		return true;
	case btEnum:
		to.put(L"enum");
		return true;
	case btLong:
		to.put(L"long");
		return true;
	case btULong:
		to.put(L"ulong");
		return true;
	case btCurrency:
		to.put(L"Currency");
		return true;
	case btDate:
		to.put(L"Date");
		return true;
	case btVariant:
		to.put(L"Variant");
		return true;
	case btComplex:
		to.put(L"Complex");
		return true;
	case btBit:
		to.put(L"bit");
		return true;
	case btBSTR:
		to.put(L"BSTR");
		return true;
	case btHresult:
		to.put(L"HRESULT");
		return true;
	}
	return false;
}


static void outputSymbol(GenericOutput &to, SymInfo &symbol);

static void outputSymbol(GenericOutput &to, ULONG64 base, DWORD index) {
	DbgHelp &h = dbgHelp();

	DWORD tag;
	SymGetTypeInfo(h.process, base, index, TI_GET_SYMTAG, &tag);
	DWORD tmp;

	switch (tag) {
	case SymTagBaseType:
		SymGetTypeInfo(h.process, base, index, TI_GET_BASETYPE, &tmp);
		baseType(to, tmp);
		break;
	case SymTagUDT: {
		SymInfo info;
		SymFromIndex(h.process, base, index, &info);
		outputSymbol(to, info);
		break;
	}
	case SymTagPointerType:
		SymGetTypeInfo(h.process, base, index, TI_GET_TYPEID, &tmp);
		outputSymbol(to, base, tmp);
		to.put(L"*");
		break;
	case SymTagFunctionArgType:
		SymGetTypeInfo(h.process, base, index, TI_GET_TYPEID, &tmp);
		outputSymbol(to, base, tmp);
		// There may be a name here somewhere!
		break;
	case SymTagFunctionType:
		to.put(L"<function ptr>");
		break;
	case SymTagEnum:
		to.put(L"<enum>");
		break;
		// ...
	default:
		to.put(L"<tag: ");
		to.put(tag);
		to.put(L">");
	}

}

// Output a symbol (with parameters).
static void outputSymbol(GenericOutput &to, SymInfo &symbol) {
	DbgHelp &h = dbgHelp();

	// Type of item:
	DWORD type;
	if (SymGetTypeInfo(h.process, symbol.ModBase, symbol.TypeIndex, TI_GET_TYPE, &type)) {
		outputSymbol(to, symbol.ModBase, type);
		to.put(L" ");
	}


	// Name
	to.put(symbol.Name);

	// Function?
	if (symbol.Tag != SymTagFunction)
		return;

	// Parameters.
	DWORD children = 0;
	SymGetTypeInfo(h.process, symbol.ModBase, symbol.TypeIndex, TI_GET_CHILDRENCOUNT, &children);
	to.put(L"(");

	byte *mem = new byte[sizeof(TI_FINDCHILDREN_PARAMS) + sizeof(ULONG)*children];
	TI_FINDCHILDREN_PARAMS *params = (TI_FINDCHILDREN_PARAMS *)mem;
	params->Count = children;
	params->Start = 0;

	SymGetTypeInfo(h.process, symbol.ModBase, symbol.TypeIndex, TI_FINDCHILDREN, params);
	for (nat i = 0; i < nat(children); i++) {
		if (i != 0)
			to.put(L", ");
		outputSymbol(to, symbol.ModBase, params->ChildId[i]);
	}

	delete []mem;

	to.put(L")");
}

void CppInfo::format(GenericOutput &to, void *fnBase, int offset) const {
	DbgHelp &h = dbgHelp();

	const nat maxNameLen = 512;
	SymInfo symbol;
	DWORD64 ptr = (DWORD64)fnBase + offset;
	DWORD64 codeDisplacement;
	bool hasSymName = SymFromAddr(h.process, ptr, &codeDisplacement, &symbol) ? true : false;

	IMAGEHLP_LINE64 line;
	DWORD lineDisplacement;
	line.SizeOfStruct = sizeof(line);
	bool hasLine = SymGetLineFromAddr64(h.process, ptr, &lineDisplacement, &line) ? true : false;

	if (hasLine) {
		Path path(line.FileName);
#ifdef DEBUG
		path = path.makeRelative(Path::dbgRoot());
#endif
		std::wostringstream p;
		p << path;
		to.put(p.str().c_str());
		to.put(L"(L");
		to.put(line.LineNumber);
		to.put(L"): ");
	} else {
		to.put(L"<unknown location>: ");
	}

	if (hasSymName) {
		outputSymbol(to, symbol);
	} else {
		to.put(L"Unknown function @");
		to.putHex((size_t)ptr);
	}
}

#elif defined(POSIX) && !defined(NO_LIBBACKTRACE)
#include "backtrace.h"
#include <cxxabi.h>
#include <setjmp.h>
#include <signal.h>

static void backtraceError(void *data, const char *msg, int errnum) {
	printf("Error during backtraces (%d): %s\n", errnum, msg);
}

class LookupState {
public:
	char name[PATH_MAX + 1];
	backtrace_state *state;

	LookupState() {
		memset(name, 0, PATH_MAX + 1);
		ssize_t r = readlink("/proc/self/exe", name, PATH_MAX);
		if (r >= PATH_MAX || r < 0)
			throw UserError(L"Failed to read the path of the current executable.");

		state = backtrace_create_state(name, 1, &backtraceError, null);
	}

};

struct FormatData {
	GenericOutput &to;
	bool any;
};

static void formatError(void *data, const char *msg, int errnum) {
	FormatData *d = (FormatData *)data;
	d->any = false;
}

static THREAD jmp_buf checkStringExit;

static void handleSigSegv(int signal) {
	(void)signal;
	siglongjmp(checkStringExit, 1);
}

// Check so that it is possible to read a string. It seems like backtrace_syminfo sometimes might
// give bogous strings to us, especially when messing with signal handlers. Note: since we mess with
// signal handlers, this will not work reliably with strings on the GC heap.
static bool checkString(const char *str) {
	struct sigaction newAction, oldAction;
	sigemptyset(&newAction.sa_mask);
	newAction.sa_handler = &handleSigSegv;
	newAction.sa_flags = SA_RESTART;
	sigaction(SIGSEGV, &newAction, &oldAction);
	if (sigsetjmp(checkStringExit, true) != 0) {
		// Segmentation fault reading the string!
		sigaction(SIGSEGV, &oldAction, NULL);

		return false;
	}

	// Try reading the string. A call to strlen works nicely. Use the result so that it is not
	// optimized out.
	bool ok =  strlen(str) > 0;
	sigaction(SIGSEGV, &oldAction, NULL);
	return ok;
}

static void formatOk(void *data, uintptr_t pc, const char *symname, uintptr_t symval, uintptr_t symsize) {
	FormatData *d = (FormatData *)data;

	// If the version of libbacktrace does not match the one GCC uses well enough, then we won't
	// crash at least. In some systems it seems like libbacktrace is installed by default.
	if (checkString(symname)) {
		int status = 0;
		char *demangled = abi::__cxa_demangle(symname, null, null, &status);
		if (status == 0) {
			d->to.put(demangled);
		} else {
			d->to.put(symname);
		}
		free(demangled);
		d->any = true;
	} else {
		d->any = false;
	}
}

void CppInfo::format(GenericOutput &to, void *base, int offset) const {
	static LookupState state;

	FormatData data = { to, false };
	backtrace_syminfo(state.state, (uintptr_t)base + offset, &formatOk, &formatError, &data);

	if (!data.any) {
		to.put("Unknown function @");
		to.putHex((size_t)base + offset);
	}
}

#elif defined(POSIX)
#include <execinfo.h>

// This gives us worse results than using libbacktrace above, but does not rely on any kind of
// symbols or external libraries.

void CppInfo::format(GenericOutput &to, void *base, int offset) const {
	void *buffer[1] = { (byte *)base + offset };
	char **data = backtrace_symbols(buffer, 1);

	if (data[0]) {
		to.put(data[0]);
	}

	::free(data);
}

#else
#error "Please implement CppInfo::format for your OS here."
#endif
