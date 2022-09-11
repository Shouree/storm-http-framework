#pragma once

#include "Gc/Config.h"

#if STORM_GC == STORM_GC_MPS

// Includes the C-library part of MPS.
// Note: This header is also included in C-mode, so we need to detect that properly.

#ifdef __cplusplus
extern "C" {
#endif

	#if defined(SLOW_DEBUG)
	// Run MPS in its cool (as opposed to hot) configuration when in debug mode.
#define CONFIG_VAR_COOL
#else
	// Run MPS in the hot configuration.
#endif

	// Use real-time signals for suspend and resume. This lessens the risk of clashes for programs
	// actually using the default SIGXFSZ and SIGXCPU.
#define CONFIG_PTHREADEXT_SIGSUSPEND (SIGRTMIN)
#define CONFIG_PTHREADEXT_SIGRESUME (SIGRTMIN+1)

	// Call a cache-invalidation function before resuming threads.
#define CONFIG_CALL_BEFORE_RESUME() mps_before_resume()

#ifdef __cplusplus
	// Do not include these if we're compiled from 'mps.c', since that will break the MPS setup of
	// defines for the system.

#include "mps/code/mps.h" // MPS core
#include "mps/code/mpslib.h" // MPS plinth
#include "mps/code/mpsavm.h" // VM arena
#include "mps/code/mpscamc.h" // AMC pool
#include "mps/code/mpscams.h" // AMS pool
#include "mps/code/mpscawl.h" // AWL pool
#include "mps/code/mpscmvff.h" // MVFF pool
#include "mps/code/mpsclo.h" // LO pool

	// Decrease/increase the # of byte scanned for this scanned set (hack).
	void mps_decrease_scanned(mps_ss_t ss, size_t decrease);
	void mps_increase_scanned(mps_ss_t ss, size_t increase);

	// Custom assertion failure handler. Calls 'DebugBreak' to aid when debugging.
	void mps_assert_fail(const char *file, unsigned line, const char *condition);

	// Setup some global state for the MPS. This is not strictly required, but it makes debugging a
	// lot easier.
	void mps_init();

}
#endif

#endif
