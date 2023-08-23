#pragma once

/**
 * Includes required to include the "mysql.h" file on the current system.
 *
 * TODO: We might want to provide a copy of the mysql.h header, since the dynamic loading means that
 * we only need the header to be present to compile Storm.
 */

#include <mariadb/mysql.h>


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
