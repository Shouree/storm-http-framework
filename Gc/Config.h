#pragma once

/**
 * Select the GC to use here.
 *
 * ZERO is just allocating memory without ever returning it. Usable as a template for new GC
 * implementations. Note: Storm will not shut down properly unless destructors are called, which the
 * ZERO gc does not do.
 *
 * MPS is currently the fastest GC supported.
 *
 * SMM is a custom GC implementation for Storm. It is not yet as well-tested as MPS.
 */

#ifdef DEBUG
// Slow but better debugging?
//#define SLOW_DEBUG
#endif

// Constants for selecting a GC.
#define STORM_GC_ZERO 0
#define STORM_GC_MPS 1
#define STORM_GC_SMM 2
#define STORM_GC_DEBUG 3
// ...

#ifndef STORM_GC

// Select the GC to use if nothing else is specified.
#define STORM_GC STORM_GC_MPS

#endif
