SQL Library
===========

The package [stormname:sql] contains a SQL library. The library consists of two main parts:

- A set of interfaces to communicate with different databases.

- A language extension to Basic Storm that allows mixing SQL with Basic Storm in a type-safe way.
  The extension optionally provides type-safety through the database layer.

As such, documentation of the SQL library is divided into two sections. The first describes the
generic database interface, and the second describes the language extension.


The library currently supports SQLite, MariaDB, and MySQL. On Linux it might be necessary to install
the `libmariadb-dev` package to connect to databases, since the dynamic library is not distributed
alongside Storm.


The SQL library was originally developed by: David Ångström, Markus Larsson, Mohammed Hamade, Oscar
Falk, Robin Gustavsson, Saima Akhter. The support for MariaDB and MySQL was later added by Hanna
Häger and Simon Westerberg Jernström.

