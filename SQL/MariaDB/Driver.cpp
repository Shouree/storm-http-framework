#include "stdafx.h"
#include "Driver.h"
#include "Exception.h"
#include "Core/Convert.h"

#ifdef POSIX
#include <dlfcn.h>
#endif

namespace sql {

	// Note: These are globals, as loading of dynamic libraries is truly a global concern. It is not
	// tied to different Engines.

	// Lock to protect globals here.
	util::Lock driverLock;

	// Instances that are active.
	size_t instances = 0;

	// Pointer to the "mysql_init" function.
	typedef MYSQL *(STDCALL *InitFn)(MYSQL *);
	InitFn mariadbInitFn = null;

#if defined(WINDOWS)

	// MSVC "hack" to get the address of the module:
	extern "C" IMAGE_DOS_HEADER __ImageBase;

	InitFn findDriverLib(Engine &e) {
#ifdef X64
		const char *fnName = "mysql_init";
#else
		const char *fnName = "_mysql_init@4";
#endif

		const char *names[] = {
			"libmariadb.dll",
#ifdef X64
			"libmariadb64.dll",
#else
			"libmariadb32.dll",
#endif
		};

		HMODULE moduleHandle = (HMODULE)&__ImageBase;

#if 0
		// Try to get the current DLL handle. Only needed if we are not compiling on MSVC.
		if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
								| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
								(LPWSTR)&findDriverLib, // ptr to some function in the modle
								&moduleHandle)) {
			throw new (e) SQLError(new (e) Str(S("Could not find the path to the SQL lib DLL file.")));
		}
#endif

		std::vector<wchar_t> buffer(MAX_PATH + 1, 0);
		do {
			GetModuleFileName(moduleHandle, &buffer[0], DWORD(buffer.size()));
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				buffer.resize(buffer.size() * 2);
				continue;
			}
		} while (false);

		// Find the last backslash:
		size_t pathEnd = buffer.size();
		while (pathEnd > 0) {
			if (buffer[pathEnd - 1] == '\\' || buffer[pathEnd - 1] == '/')
				break;
			pathEnd--;
		}

		// Try all names:
		for (size_t i = 0; i < ARRAY_COUNT(names); i++) {
			buffer.resize(pathEnd + strlen(names[i]) + 1);
			for (size_t j = 0; names[i][j]; j++)
				buffer[pathEnd + j] = names[i][j];

			HMODULE lib = LoadLibrary(&buffer[0]);
			if (!lib)
				continue;

			void *fn = GetProcAddress(lib, fnName);
			if (!fn) {
				FreeLibrary(lib);
				continue;
			}

			return reinterpret_cast<InitFn>(fn);
		}

		// We failed. Generate a nice error message!
		StrBuf *msg = new (e) StrBuf();
		*msg << S("Failed to load the MariaDB library. Looked in the following locations:");

		for (size_t i = 0; i < ARRAY_COUNT(names); i++) {
			buffer.resize(pathEnd + strlen(names[i]) + 1);
			for (size_t j = 0; names[i][j]; j++)
				buffer[pathEnd + j] = names[i][j];

			*msg << S("\n") << &buffer[0];
		}
		throw new (e) SQLError(msg->toS());
	}

#elif defined(POSIX)

	InitFn findDriverLib(Engine &e) {
		const char *fnName = "mysql_init";
		const char *names[] = {
			"libmariadb.so.3",
			null
		};

		const char *libName = null;
		void *loaded = null;
		for (const char **name = names; loaded == null && *name != null; name++) {
			// Note: This respects rpath of the library if we set it.
			libName = *name;
			loaded = dlopen(libName, RTLD_NOW | RTLD_LOCAL);
		}

		if (!loaded) {
			StrBuf *msg = new (e) StrBuf();
			*msg << S("Failed to load the MariaDB library. Is it installed?\n");
			*msg << S("Searched for the following names:");
			for (const char **name = names; loaded == null && *name != null; name++) {
				*msg << S("\n") << toWChar(e, *name)->v;
			}

			throw new (e) SQLError(msg->toS());
		}

		void *symbol = dlsym(loaded, fnName);

		if (!symbol) {
			StrBuf *msg = new (e) StrBuf();
			*msg << S("Failed to find the function ") << toWChar(e, fnName)->v
				 << S(" in the shared library ") << libName << S(".");
			throw new (e) SQLError(msg->toS());
		}

		return reinterpret_cast<InitFn>(symbol);
	}

#endif

	MYSQL *createDriver(Engine &e) {
		util::Lock::L z(driverLock);

		if (!mariadbInitFn)
			mariadbInitFn = findDriverLib(e);

		MYSQL *result = (*mariadbInitFn)(NULL);
		if (!result)
			throw new (e) SQLError(new (e) Str(S("Failed to create an instance of the MySQL/MariaDB driver.")));

		instances++;
		return result;
	}

	void destroyDriver(MYSQL *driver) {
		// Close it.
		(*driver->methods->api->mysql_close)(driver);

		util::Lock::L z(driverLock);
		instances--;
		// TODO: We don't want to unload immediately, but might be relevant to unload after a while
		// of inactivity?
	}

}
