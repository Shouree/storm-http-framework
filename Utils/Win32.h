#pragma once
#include "Platform.h"

/**
 * Our version of including Windows.h, we're modifying the behavior a bit.
 */

#ifdef WINDOWS
#define WIN32_LEAN_AND_MEAN
// We don't want Windows.h to define min and max as macros!
#define NOMINMAX

// Target Windows Vista and above.
#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include <Windows.h>

// Sometimes it re-defines 'small' to 'char', no good!
#ifdef small
#undef small
#endif


#endif


#ifdef POSIX
#include <pthread.h>
#include <semaphore.h>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, const char *argv[]);

#endif
