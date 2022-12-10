#include "stdafx.h"
#include "Utils/Mode.h"
#include "Utils/Platform.h"
#include "Utils/Cache.h"
#include "Lib.h"

#if STORM_GC == STORM_GC_MPS

void mps_before_resume();

#pragma warning(disable: 4068) // Do not warn about unknown pragmas.
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wcast-function-type"

/**
 * Note: Defining CONFIG_VAR_COOL or CONFIG_VAR_RASH from mymake will compile the 'cool' or the
 * 'rash' variety of MPS as opposed to the 'hot' variety. The cool variety will do more checking on
 * the hot path of MPS, while the rash variety does no checking and contains no telemetry output.
 */

#include "mps/code/mps.c" // Includes all code needed for MPS.

// Helper function that abuses the internals of MPS a bit.
void mps_decrease_scanned(mps_ss_t mps_ss, size_t decrease) {
	ScanState ss = PARENT(ScanStateStruct, ss_s, mps_ss);
	ss->scannedSize -= decrease;
}

void mps_increase_scanned(mps_ss_t mps_ss, size_t increase) {
	ScanState ss = PARENT(ScanStateStruct, ss_s, mps_ss);
	ss->scannedSize += increase;
}

void mps_before_resume() {
	clearLocalICache();
	dataBarrier();
}

void gc_panic_stacktrace(void);

// Custom assertion failure handler. Calls 'DebugBreak' to aid when debugging.
void mps_assert_fail(const char *file, unsigned line, const char *condition) {
	fflush(stdout);
	fprintf(stderr,
			"The MPS detected a problem!\n"
			"%s:%u: MPS ASSERTION FAILED: %s\n",
			file, line, condition);
	fflush(stderr);
	mps_telemetry_flush();
#ifdef DEBUG
#ifdef WINDOWS
	DebugBreak();
#else
	raise(SIGINT);
#endif
#endif
	abort();
}

#ifdef POSIX
void mps_on_sigsegv(int signal) {
	(void)signal;
	gc_panic_stacktrace();
	// Raise SIGINT instead. See the comment below for details.
	raise(SIGINT);
}
#endif

void mps_init() {
	// Custom assertions.
	mps_lib_assert_fail_install(&mps_assert_fail);
}

#endif
