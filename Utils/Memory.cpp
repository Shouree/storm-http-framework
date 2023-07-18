#include "stdafx.h"
#include "Memory.h"

#ifdef WINDOWS

// Is the address readable?
bool readable(const void *addr) {
	if (addr == null)
		return false;

	MEMORY_BASIC_INFORMATION info;
	SIZE_T r = VirtualQuery(addr, &info, sizeof(info));
	if (r != sizeof(info))
		return false;

	if (info.State == MEM_FREE)
		return false;

	if (info.Protect == 0)
		return false;
	if (info.Protect & PAGE_NOACCESS)
		return false;
	if (info.Protect & PAGE_GUARD)
		return false;
	if (info.Protect & PAGE_EXECUTE) // I think this is execute only.
		return false;

	return true;
}

// Print some information about 'addr' for debugging.
void memFlags(const void *addr) {
	MEMORY_BASIC_INFORMATION info;
	SIZE_T r = VirtualQuery(addr, &info, sizeof(info));
	assert(r == sizeof(info));

	printf("%p:", addr);
	if (info.State == MEM_FREE)
		printf(" free");
	if (info.Protect & PAGE_NOACCESS || info.Protect == 0)
		printf(" noaccess");
	if (info.Protect & PAGE_GUARD)
		printf(" guard");
	if (info.Protect & PAGE_EXECUTE) // I think this is execute only.
		printf(" executable");
	if (info.Protect & PAGE_READONLY)
		printf(" readonly");
	if (info.Protect & PAGE_READWRITE)
		printf(" readwrite");

	printf("\n");
}

#endif
