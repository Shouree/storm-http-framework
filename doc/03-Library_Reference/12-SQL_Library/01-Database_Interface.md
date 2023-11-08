Database Interface
==================

The generic database interface consists of the following interfaces that allow making queries to
databases. The library also provides implementations for SQLite, MariaDB, and MySQL.

The classes in the database interface are not tied to any particular thread. They are, however, not
designed to be used by multiple threads concurrently.


Query Strings
-------------

To help users of the library account for differences between different databases, the library
provides the [stormname:sql.QueryStr] class. The class can be thought of as a regular string, but
some parts of the string have semantic information attached to them. For example, the `QueryStr`
class knows that a certain part is supposed to be a placeholder, a name, or a type. It is then able
to generate the proper escape sequences for the database that is used based on this information
(even though SQL is standardized, not all databases follow the standard).

A `QueryStr` is created by using a [stormname:sql.QueryStrBuilder]. The builder class has the
following members:

```stormdoc
@sql.QueryStrBuilder
- .__init()
- .put(*)
- .name(*)
- .placeholder()
- .autoIncrement()
- .type(*)
- .build()
- .clear()
```

Query Types
-----------

Different databases support different data types. As such, the library provides the type
[stormname:sql.QueryType] that provides a generic representation of these types, and aims to provide
some resilience against a database modifying a generic type into a more specific one.

The `QueryType` supports three types currently: text, integer, and real. These types may
additionally have a size attached to them.

The `QueryType` has the following members:

```stormdoc
@sql.QueryType
- .__init()
- .text()
- .integer()
- .real()
- .parse(*)
- .sized(*)
- .size()
- .sameType(*)
- .compatible(*)
- .any()
- .empty()
```


Connection
----------

The class [stormname:sql.DBConnection] represents a connection to a database. The class itself is
abstract, so it is necessary to use the appropriate derived class to connect to a database.

The `DBConnection` class has the following members:

```stormdoc
@sql.DBConnection
- .prepare(*)
- .close()
- .tables()
- .schema(*)
```

### Database Types

The following classes can be created to connect to different databases:

- [stormname:sql.SQLite] - SQLite. Supports both in-memory databases and databases in files in the local file system.

- [stormname:sql.MariaDB] - MariaDB. Connects to a database as specified in the `Host` object (supports both TCP and UNIX sockets).

- [stormname:sql.MySQL] - MySQL. Connects to a database as specified in the `Host` object (supports both TCP and UNIX sockets).


Statement
---------

The [stormname:sql.Statement] class represents a prepared statement, that is a statement that
possibly contains placeholders for values. The `Statement` class contains the `bind` function to
allow binding values to the placeholders, and allows executing the query. In general, it is a good
idea to retain the prepared statement for as long as possible, since this means that the database
does not have to re-compile the query every time.

As mentioned above, parameters are bound using the `bind(Nat pos, T value)` overloads. There are
also free functions that allow binding `Maybe<T>` for the supported types. Finally, there is
`bindNull(Nat pos)` to explicitly set a placeholder to `null`.

Apart from `bind`, the `Statement` class has the following parameters:

```stormdoc
@sql.Statement
- .execute()
- .finalize()
```


Result
------

When a statement has been executed, it returns a [stormname:sql.Statement.Result] class that
represents a cursor to the result. The result class fetches the result rows lazily, so that large
result sets can be managed without running out of memory. This does, however, mean that it is not
possible to modify the parameters bound to the `Statement` while iterating through a result produced
by the same `Statement`. It is, however, possible to modify other `Statement`s while consuming the
results from a statement (note: this might however cause some databases to cache all results, since
the connection can not always be multiplexed).

The `Result` class has the following members:

```stormdoc
@sql.Statement.Result
- .next()
- .lastRowId()
- .changes()
- .finalize()
```

It is possible to use a result inside a for-loop in Basic Storm due to the presence of the `next`
(and `iter`) members.

Note that since the `Result` is a value, it is not necessary to call `finalize()`. This will be done
automatically as soon as the `Result` (and all copies) go out of scope. It is, however, possible to
finalize it prematurely in cases where that is more convenient.


Row
---

```stormdoc
sql.Row
- .count()
- .getStr(*)
- .getBool(*)
- .getInt(*)
- .getLong(*)
- .getFloat(*)
- .getDouble(*)
- .isNull(*)
- sql.at(sql.Row, core.Nat)
```


Example
-------

As an example, one can use the database library to make a simple query to an SQL database as follows:

```bs
use sql;
use core:io;

void main() {
    SQLite db(cwdUrl / "file.db");

    var stmt = db.prepare("SELECT * FROM test WHERE id > ?;");
    stmt.bind(0, 18);

    for (row in stmt) {
        print(row.getStr(0));
    }
}
```
