#pragma once

/**
 * Includes required to include the "mysql.h" file on the current system.
 *
 * On Linux, we typically use the header found in the system. On Windows, we use (and compile) a
 * copy of the library ourselves.
 */

#ifdef MARIADB_LOCAL_DRIVER
#include "Windows/mariadb/include/mysql.h"
#include "Windows/mariadb/include/mysqld_error.h"
#else

// This is available in C++17 and above:
#if __has_include(<mariadb/mysql.h>)
#include <mariadb/mysql.h>
#include <mariadb/mysqld_error.h>
#else
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#endif

#endif


namespace sql {

	/**
	 * Allow loading the mariadb/mysql driver dynamically when needed.
	 *
	 * This means that we only need the dynamic library to be present when we actually need it.
	 */

	// Create an instance of the MYSQL object. This can then be used to access the entire
	// MySQL/MariaDB API. Loads the mariadb library as needed.
	MYSQL *createDriver(Engine &e);

	// Destroy an instance of a driver. This allows the implementation to reference count and
	// determine when/if to unload the mysql library.
	void destroyDriver(MYSQL *driver);

}
