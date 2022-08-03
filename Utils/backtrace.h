#pragma once
#include "Platform.h"

#if defined(POSIX) && !defined(NO_LIBBACKTRACE)

// Note: On POSIX, there is a function named 'backtrace_symbols', which seems promising. However, it
// just uses 'dladdr' to find function names, which is not sufficient since it does not get names
// for functions inside the current binary.

// Defines from 'config.h'
#if defined(X86)
#define BACKTRACE_ELF_SIZE 32
#elif defined(X64)
#define BACKTRACE_ELF_SIZE 64
#elif defined(ARM64)
#define BACKTRACE_ELF_SIZE 64
#else
#error "Backtrace elf size not known."
#endif

#define HAVE_ATOMIC_FUNCTIONS 1
#define HAVE_DECL_STRNLEN 1
#define HAVE_DL_ITERATE_PHDR 1
#define HAVE_FCNTL 1
#define HAVE_SYNC_FUNCTIONS 1

/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

#include "Linux/backtrace/backtrace.h"
#endif
