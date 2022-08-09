#include "stdafx.h"
#include "Stack.h"
#include "Exception.h"
#include "Utils/Bitwise.h"

#ifdef POSIX
#include <sys/mman.h>
#endif


namespace os {

	Stack::Stack(size_t size) : desc(null), detourActive(0), detourTo(null), alloc(null), size(0) {
		allocate(size);
		initDesc();
	}

	Stack::Stack(void *base) : desc(null), detourActive(0), detourTo(null), alloc(base), size(0) {
		// Nothing more to do!
	}

	Stack::~Stack() {
		if (size > 0) {
			free();
			size = 0;
		}
	}

	void Stack::clear() {
		initDesc();
	}


#if defined(WINDOWS)

	static size_t pageSize() {
		static size_t sz = 0;
		if (sz == 0) {
			SYSTEM_INFO sysInfo;
			GetSystemInfo(&sysInfo);
			sz = sysInfo.dwPageSize;
		}
		return sz;
	}

	void Stack::allocate(size_t size) {
		size_t pageSz = pageSize();
		size = roundUp(size, pageSz);
		size += pageSz; // We want a guard page.

		byte *mem = (byte *)VirtualAlloc(null, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (mem == null) {
			// TODO: What to do in this case?
			throw ThreadError(L"Out of memory when spawning a thread.");
		}

		DWORD oldProt;
		VirtualProtect(mem, 1, PAGE_READONLY | PAGE_GUARD, &oldProt);

		// Do not show the guard page to other parts...
		this->alloc = mem + pageSz;
		this->size = size - pageSz;
	}

	void Stack::free() {
		byte *mem = (byte *)alloc;
		mem -= pageSize();
		VirtualFree(mem, 0, MEM_RELEASE);
	}

	void Stack::initDesc() {
		// Put the initial stack description in the 'top' of the stack.
		// It will be updated when we initialize the stack later.
		byte *r = (byte *)alloc;
		desc = (Desc *)alloc;

		desc->low = r + size;
		desc->dummy = r;
		desc->high = r + size;
	}

#elif defined(POSIX)

	static size_t pageSize() {
		static size_t sz = 0;
		if (sz == 0) {
			int s = getpagesize();
			assert(s > 0, L"Failed to acquire the page size for your system!");
			sz = (size_t)s;
		}
		return sz;
	}

	void Stack::allocate(size_t size) {
		size_t pageSz = pageSize();
		size = roundUp(size, pageSz);
		size += pageSz; // We want a guard page.

		byte *mem = (byte *)mmap(null, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (mem == null) {
			// TODO: What to do in this case?
			throw ThreadError(L"Out of memory when spawning a thread.");
		}

		mprotect(mem, 1, PROT_NONE); // no special guard-page it seems...

		// Do not show the guard page to other parts...
		this->alloc = mem + pageSz;
		this->size = size - pageSz;
	}

	void Stack::free() {
		byte *mem = (byte *)alloc;
		size_t pageSz = pageSize();
		mem -= pageSz;
		munmap(mem, size + pageSz);
	}

	void Stack::initDesc() {
		// Put the initial stack description in the 'top' of the stack.
		// It will be updated when we initialize the stack later.
		byte *r = (byte *)alloc;
		desc = (Desc *)alloc;

		desc->low = r + size;
		desc->dummy = r;
		desc->high = r + size;
	}


#endif

}
