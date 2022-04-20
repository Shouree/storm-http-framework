#pragma once

/**
 * This file contains macros for finding out which platform
 * we are currently being compiled on. This file is designed
 * to not depend on anything, and can therefore safely be included
 * from anywhere.
 */

/**
 * Machine type defines:
 * X86 - x86 cpu
 * X64 - x86-64/amd64 cpu
 */

/**
 * Platform defined:
 * LINUX - compiled on linux
 * POSIX - compiled on posix-compatible platform (eg linux)
 * WINDOWS - compiled on windows
 */

/**
 * Endian-ness: Look in Endian.h for more helpers regarding endianness.
 * LITTLE_ENDIAN
 * BIG_ENDIAN
 */


/**
 * Compilers:
 * VISUAL_STUDIO - Visual Studio compiler. Set to the version, eg 2008 for VS2008.
 * GCC - GCC compiler (or compatible), TODO: version?
 */

/**
 * Compiler features:
 * USE_MOVE - is move semantics supported?
 * USE_VA_TEMPLATE - is variable templates supported?
 */

/**
 * Standardized function/variable specifications:
 * THREAD - thread local variable
 * NAKED - naked function call, ie no prolog/epilog. Maybe not supported for all compilers.
 */

/**
 * CODECALL:
 * Calling convention used in the Code lib. This is not inside the Code lib, since it will make
 * mymake think that Code is dependent on almost all projects, since they only need the CODECALL macro.
 */

/**
 * ALIGN_AS(x):
 * Specify alignment for a structure. Only expect this to work for automatic allocations, i.e. not
 * for allocations using "new", since this macro may expand to a nonstandard extension.
 */

/**
 * SHARED_EXPORT:
 * Used to mark functions that shall be exported from a shared library.
 */

/**
 * EXCEPTION_EXPORT:
 * Used to mark all exceptions in the system, as some systems require special knowledge of all
 * classes that are used as exceptions.
 * See https://gcc.gnu.org/wiki/Visibility for details.
 */

/**
 * NOINLINE:
 * Disable inlining of the specified function. Mostly used while debugging.
 */



// Detect the current architecture and platform.
#if defined(_WIN64)
#define X64
#define WINDOWS
#elif defined(_WIN32)
#define X86
#define WINDOWS
#elif defined(__linux__)
#define LINUX
#define POSIX

#if defined(__amd64__)
#define X64
#elif defined(__i386__)
#define X86
#else
#error "Unknown (and probably unsupported) architecture for Linux."
#endif

#else
#error "Unknown platform, please add it here!"
#endif

// Detect the current compiler.
#if defined(_MSC_VER)
// Visual Studio compiler!
#if _MSC_VER >= 1923
#define VISUAL_STUDIO 2019
#elif _MSC_VER >= 1900
#define VISUAL_STUDIO 2015
#elif _MSC_VER >= 1800
#define VISUAL_STUDIO 2013
#elif _MSC_VER >= 1700
#define VISUAL_STUDIO 2012
#elif _MSC_VER >= 1600
#define VISUAL_STUDIO 2010
#elif _MSC_VER >= 1500
#define VISUAL_STUDIO 2008
#else
#error "Too early VS version, earliest supported is VS2008"
#endif

#elif defined(__GNUC__)
// GCC
#define GCC __GNUC__

#endif

#ifdef VISUAL_STUDIO
#define THREAD __declspec(thread)
#define NAKED __declspec(naked)
#define SHARED_EXPORT __declspec(dllexport)
#define ALIGN_AS(x) __declspec(align(x))
#define EXCEPTION_EXPORT
#define NOINLINE __declspec(noinline)

#if VISUAL_STUDIO >= 2010
#define USE_MOVE // Not sure about this one...
#define USE_VA_TEMPLATE
#endif
#endif

#ifdef GCC
#define THREAD __thread
#define NAKED error // not supported.
#define SHARED_EXPORT __attribute__((visibility ("default")))
#define EXCEPTION_EXPORT __attribute__((visibility ("default")))
#define NOINLINE __attribute__((noinline))

// We require support for these features.
#define USE_MOVE
#define USE_VA_TEMPLATE
#define ALIGN_AS(x) alignas(x)
#endif

#if defined(X86) || defined(X64)
#define LITTLE_ENDIAN
#else
#error "Unknown endianness for your platform. Define either LITTLE_ENDIAN or BIG_ENDIAN here."
#endif

#ifndef THREAD
#error "someone forgot to declare THREAD for your architecture"
#endif

#ifndef NAKED
#error "someone forgot to declare NAKED for your architecture"
#endif

#ifndef SHARED_EXPORT
#error "someone forgot to declare SHARED_EXPORT for your architecture"
#endif

#if defined(VISUAL_STUDIO) && defined(X86)
#define CODECALL __cdecl
#endif

#if defined(GCC) && defined(X86)
#define CODECALL __attribute__((cdecl))
#endif

#if defined(VISUAL_STUDIO) && defined(X64)
// X86-64 does not need to specify a calling convention. There is a single standard convention!
#define CODECALL
#endif

#if defined(GCC) && defined(X64)
// X86-64 does not need to specify a calling convention. There is a single standard convention!

// We're using -falign-functions=2, but that seems to be ignored for static template functions on
// GCC 8.1.0, so we specify it here as well to be safe.
#define CODECALL  __attribute__((aligned(2)))
#endif

// Make sure it is defined.
#ifndef CODECALL
#error "Someone forgot to declare CODECALL for your architecture."
#endif
