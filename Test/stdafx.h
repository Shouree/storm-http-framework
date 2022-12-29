// #pragma once // GCC issues a warning when using 'pragma once' with precompiled headers...
#ifndef TEST_PCH
#define TEST_PCH

#include "Utils/Utils.h"
#include "Core/Storm.h"
#include "Core/Str.h"
#include "Core/StrBuf.h"
#include "Compiler/Storm.h"
#include "Compiler/Engine.h"

using namespace storm;

// Get the global engine object.
Engine &gEngine();

// Get the global GC object.
Gc &gc();

#include "Test/Lib/Test.h"

/**
 * Define test suites so we can place tests in a somewhat cronological order, so when porting we can
 * start worrying about the low-level parts and then build ourselves up to the final thing.
 */


// Basic threading, no GC yet.
SUITEX(OS, 0);
// Basic gc operation, scanning simple objects without anything strange.
SUITEX(GcScan, 1);
// Gc operation on Storm objects declared in C++.
SUITEX(GcObjects, 2);
// GC along with threads.
SUITEX(GcThreads, 3);
// Basic operation of the runtime. No code generation yet.
SUITE(Core, 4);
// Basic tests of the code generation backend. Further tests rely on these to work.
SUITE(CodeBasic, 5);
// Tests of the code generation backend.
SUITE(Code, 6);
// More tests of the code generation backend. Requires code generation to function.
SUITE(CoreEx, 7);
// More involved tests of Storm. Still no compilation from source code.
SUITE(Storm, 8);
// Simple tests of Basic Storm
SUITE(SimpleBS, 9);
// More heavy tests of Basic Storm
SUITE(BS, 10);
// Some compilation tests that make sure the provided code compiles. Nice to have these last.
SUITE(Compile, 11);
// Language server logic.
SUITE(Server, 12);

// UI library. Sadly not too many automated tests.
SUITED(Gui, 20);

// Stress tests (takes a couple of minutes, too slow to be included always).
SUITED(Stress, 100);


/**
 * Helpers for comparing objects in Storm.
 */

template <class T, class U>
void verifyObjEq(TestResult &r, T *lhs, U *rhs, const String &expr) {
	if (!(*lhs == *rhs)) {
		r.failed++;
		std::wcout << L"Failed: " << expr << L" == " << lhs << L" != " << rhs << std::endl;
	}
}

#define CHECK_OBJ_EQ_TITLE(expr, eq, title)						\
	try {														\
		__result__.total++;										\
		std::wostringstream __stream__;							\
		__stream__ << title;									\
		verifyObjEq(__result__, expr, eq, __stream__.str());	\
	} catch (const storm::Exception *e) {						\
		OUTPUT_ERROR(title, e);									\
	} catch (const ::Exception &e) {							\
		OUTPUT_ERROR(title, e);									\
	} catch (...) {												\
		OUTPUT_ERROR(title, "unknown crash");					\
	}


#define CHECK_OBJ_EQ(expr, eq) CHECK_OBJ_EQ_TITLE(expr, eq, #expr)


// Call a function and check all extra register so they are properly preserved.
int callFn(const void *ptr, int v);
int64 callFn(const void *ptr, int64 v);


class Error : public ::Exception {
public:
	String what() const { return L"Test error"; }
};


#ifdef POSIX
#define Sleep(X) usleep((X) * 1000LL);
#endif

#endif
