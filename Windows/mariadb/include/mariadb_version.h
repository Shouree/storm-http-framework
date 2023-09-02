/* Copyright Abandoned 1996, 1999, 2001 MySQL AB
   This file is public domain and comes with NO WARRANTY of any kind */

/* Version numbers for protocol & mysqld */

#ifndef _mariadb_version_h_
#define _mariadb_version_h_

#ifdef _CUSTOMCONFIG_
#include <custom_conf.h>
#else
#define PROTOCOL_VERSION		10 // @PROTOCOL_VERSION@
#define MARIADB_CLIENT_VERSION_STR	"10.6.8" // "@MARIADB_CLIENT_VERSION@"
#define MARIADB_BASE_VERSION		"mariadb-10.6" // "@MARIADB_BASE_VERSION@"
#define MARIADB_VERSION_ID		(10 * 10000 + 6 * 100 + 8) // @MARIADB_VERSION_ID@
#define MARIADB_PORT	        	3306 // @MARIADB_PORT@
#define MARIADB_UNIX_ADDR               "/tmp/mysql.sock" // "@MARIADB_UNIX_ADDR@"
#ifndef MYSQL_UNIX_ADDR
#define MYSQL_UNIX_ADDR MARIADB_UNIX_ADDR
#endif
#ifndef MYSQL_PORT
#define MYSQL_PORT MARIADB_PORT
#endif

#define MYSQL_CONFIG_NAME               "my"
#define MYSQL_VERSION_ID                MARIADB_VERSION_ID // @MARIADB_VERSION_ID@
#define MYSQL_SERVER_VERSION            MARIADB_CLIENT_VERSION_STR "-MariaDB" // "@MARIADB_CLIENT_VERSION@-MariaDB"

#define MARIADB_PACKAGE_VERSION "3.2.7" // "@CPACK_PACKAGE_VERSION@"
#define MARIADB_PACKAGE_VERSION_ID (3 * 10000 + 2 * 100 + 7) // @MARIADB_PACKAGE_VERSION_ID@

#define MARIADB_SYSTEM_TYPE "Windows" // "@CMAKE_SYSTEM_NAME@"
#ifdef _WIN64
#define MARIADB_MACHINE_TYPE "AMD64" // "@CMAKE_SYSTEM_PROCESSOR@"
#else
#define MARIADB_MACHINE_TYPE "x86" // "@CMAKE_SYSTEM_PROCESSOR@"
#endif
// #define MARIADB_PLUGINDIR "@CMAKE_INSTALL_PREFIX@/@INSTALL_PLUGINDIR@"

/* mysqld compile time options */
#ifndef MYSQL_CHARSET
#define MYSQL_CHARSET			"utf8mb4" // "@default_charset@"
#endif
#endif

/* Source information */
#define CC_SOURCE_REVISION "" // "@CC_SOURCE_REVISION@"

#endif /* _mariadb_version_h_ */
