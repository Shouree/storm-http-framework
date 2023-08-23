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

	InitFn findDriverLib(Engine &e) {
		throw new (e) SQLError(L"Can not load the MariaDB driver on Windows yet!");
	}

#elif defined(POSIX)

	InitFn findDriverLib(Engine &e) {
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

		const char *fnName = "mysql_init";
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
