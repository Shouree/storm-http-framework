#include "stdafx.h"
#include "VTable.h"
#include "Utils/Memory.h"

#ifdef POSIX
#include <link.h>
#endif

namespace storm {

	namespace vtable {

		const Nat invalid = -1;

#if defined(VISUAL_STUDIO) && (defined(X86) || defined(X64))

		// Visual Studio stores something at offset -1, so we use -2!
		const Nat extraOffset = 2;
		const Nat dtorOffset = 0;

		static void dumpCode(const byte *ptr, Nat size) {
			for (Nat i = 0; i < size; i++) {
				if (i % 5 != 0)
					PNN(" ");
				PNN(toHex(ptr[i]));
				if ((i + 1) % 5 == 0)
					PLN("");
			}
			if (size % 5 != 0)
				PLN("");
		}

		// In debug mode, Visual Studio links all functions to a global table of "jmp X" (probably
		// to make incremental linking easier). We need to detect and follow this! Returns 'code' if
		// is not a 'jmp' instruction.
		static const void *followJmp(const void *code) {
			const byte *c = (const byte *)code;

			if (c[0] == 0xE9) {
				// Found you!
				const Nat *rel = (const Nat *)(c + 1);
				return c + *rel + 5;
			} else {
				return code;
			}
		}

		Nat fnSlot(const void *fn) {
			fn = followJmp(fn);

			// This is the code the compiler generates for a vtable call using cdecl calling
			// convention (VS2008). We simply need to read the machine code and verify it, and then
			// extract the last byte/4 bytes and divide them by sizeof(void *)
			static const byte fnData[] = {
				// mov eax, DWORD PTR [esp+4]
				0x8B, 0x44, 0x24, 0x04,
				// mov eax, DWORD PTR [eax]
				0x8B, 0x00,
				// jmp DWORD PTR [eax+XX]
				0xFF, //0x60, 0xXX
				// or [eax+XXXXXXXX]
				// 0xFF, 0xA0, 0xXX, 0xXX, 0xXX, 0xXX
			};

			const byte *data = (const byte *)fn;
			Nat size = ARRAY_COUNT(fnData);
			if (memcmp(fnData, data, size) != 0) {
				return invalid;
				// For debugging:
				PLN(L"Mismatched machine code. Expected:");
				dumpCode(fnData, size);
				PLN(L"Got:");
				dumpCode(data, size);
				return invalid;
			}

			if (data[size] == 0x60) {
				// one byte version
				return Nat(data[size + 1]) / sizeof(void *);
			} else if (data[size] == 0xA0) {
				// four byte version
				const Nat *ptr = (const Nat *)(data + size + 1);
				return *ptr / sizeof(void *);
			} else if (data[size] == 0x20) {
				// zero byte version, always jumps to offset 0
				return 0;
			} else {
				PLN(L"Machine code mismatch: Expected 0x60 or 0xA0, got " << toHex(data[size]));
				DebugBreak();
				return invalid;
			}
		}

		// Crude but useful heurustic to find the start of the next vtable.
		static bool isNewVTable(const void *addr) {
			const Nat *p = (const Nat *)addr;
			if (p[0] < 0xFF && p[1] < 0xFF)
				return true;
			return false;
		}

		Nat count(const void *vtable) {
			const void *const* table = (const void *const*)vtable;
			assert(readable(table));

			// This is a table of pointers, so we can find the size by scanning until we find
			// something which does not look like a pointer.
			Nat size = 1;
			while (readable(table + size)) {
				// For debugging, try:
				// memFlags(table + size);
				// memFlags(table[size]);

				// TODO: Check if the address is aligned?
				// TODO: Make heurustic more similar to what is used on Linux if possible.

				if (!readable(table[size]))
					return size;

				if (isNewVTable(table[size]))
					return size;

				size++;
			}

			return size - 1;
		}

#elif defined(GCC) && defined(POSIX) && (defined(X64) || defined(ARM64)) // might hold for other architectures as well...

		// GCC stores a zero (probably base offset or similar) and type info at the two first
		// indices (as -2 and -1). We use -3!
		const Nat extraOffset = 3;
		// NOTE: There seem to bet two destructors in the VTable. I have not yet investigated the
		// difference between them.
		const Nat dtorOffset = 1;


		// GCC defines these. They are located at the start and end of the text section,
		// respectively. It is not useful to read from these, only their addresses are useful.
		extern "C" const char __executable_start;
		extern "C" const char __etext;
		extern "C" const char _edata;

		enum AddrInfo {
			addrUnknown,
			addrText,
			addrData,
		};

		// Is the address in the text section of the current executable?
		static inline bool inMyText(const void *addr) {
			const char *a = (const char *)addr;
			return &__executable_start <= a
				&& a < &__etext;
		}

		// Is the address in the data section of any loaded library?
		static inline bool inMyData(const void *addr) {
			const char *a = (const char *)addr;
			return &__etext <= a
				&& a <= &_edata;
		}

		struct CheckData {
			const void *addr;
			AddrInfo result;
		};

		// Callback from 'dl_iterate_phdr'.
		static int checkLibrary(struct dl_phdr_info *info, size_t size, void *voidData) {
			CheckData *data = (CheckData *)voidData;
			size_t addr = (size_t)data->addr;

			// Early out?
			if (addr < info->dlpi_addr)
				return 0;

			const ElfW(Phdr) *found = null;
			for (size_t i = 0; i < info->dlpi_phnum; i++) {
				const ElfW(Phdr) *header = &info->dlpi_phdr[i];
				size_t start = info->dlpi_addr + header->p_vaddr;
				size_t end = start + header->p_memsz;

				if (addr >= start && addr < end) {
					found = header;
					break;
				}
			}

			if (!found)
				return 0;

			if (found->p_type == PT_LOAD) {
				if (found->p_flags & PF_X)
					data->result = addrText;
				else
					data->result = addrData;
			}

			return 0;
		}

		// Get information on the address provided.
		// TODO: We might want to provide some kind of cache of the data we get when calling 'dl_iterate_phdr',
		// since that call is potentially expensive.
		static AddrInfo addrInfo(const void *addr) {
			if (inMyText(addr))
				return addrText;
			if (inMyData(addr))
				return addrData;

			// See if it is inside a dynamic library.
			CheckData result = {
				addr,
				addrUnknown
			};
			dl_iterate_phdr(&checkLibrary, &result);
			return result.result;
		}


		Nat fnSlot(const void *fn) {
			// See if the pointer is odd. Then it contains the offset into the vtable + 1.
			size_t ptr = (size_t)fn;
			if ((ptr & 0x1) != 0 && addrInfo(fn) != addrText) {
				return Nat((ptr - 1) / sizeof(void *));
			} else {
				return invalid;
			}
		}

		Nat count(const void *vtable) {
			const void *const* table = (const void *const*)vtable;
			assert(addrInfo(table) == addrData);

			// This is a table of pointers, so we can find the size by scanning until we find
			// something which does not look like a pointer. We also know that all member functions
			// are aligned at even addresses at the very least. Since vtables generally start with
			// null or something that is not code, we can use that to find the end of the VTable.

			// The layout of VTables for GCC seems to be (at least in our case):
			// -2: Some kind of offset, probably for multiple inheritance.
			// -1: Type information.
			// 0: Destructor (scalar or array)
			// 1: Destructor (scalar or array)
			// 2: Function 1.

			// Since the destructors might be null in some cases (abstract classes), we always assume they are there.

			Nat size = 2;
			while (addrInfo(table + size) == addrData) {
				const void *entry = table[size];

				if (size_t(entry) & 0x1)
					return size;
				if (addrInfo(entry) != addrText)
					return size;

				size++;
			}

			return size + 1;
		}

#else
#error "I do not know how VTables work on your machine!"
#endif

#if defined(VISUAL_STUDIO) || defined(GCC)

		const void *from(const RootObject *object) {
			const void *const*o = (const void *const*)object;
			return o[0];
		}

		void set(const void *vtable, RootObject *to) {
			const void **o = (const void **)to;
			o[0] = vtable;
		}

#else
#error "I do not know how VTables work on your compiler!"
#endif

		const void *deVirtualize(const void *vtable, const void *fnPtr) {
			Nat id = fnSlot(fnPtr);
			if (id == invalid)
				return null;
			const void *const*v = (const void *const*)vtable;
			return v[id];
		}

		Nat find(const void *vtable, const void *fn) {
			return find(vtable, fn, 0);
		}

		Nat find(const void *vtable, const void *fn, Nat size) {
			Nat slot = fnSlot(fn);
			if (slot != invalid)
				return slot;

			if (size == 0)
				size = count(vtable);

			const void *const*v = (const void *const*)vtable;
			Nat result = invalid;
			for (Nat i = 0; i < size; i++) {
				if (v[i] == fn) {
					if (result != invalid) {
						WARNING(L"Multiple entries with the same address in the VTable!");
						WARNING(L"Please use a non de-virtualized pointer instead!");
					} else {
						// Prefer the first match if multiple matches exist. Sometimes vtables are
						// close together so the second match is probably another vtable anyway.
						result = i;
					}
				}
			}

			return result;
		}

		const void *slot(const void *vtable, Nat slot) {
			const void *const*v = (const void *const*)vtable;
			return v[slot];
		}

	}

}
