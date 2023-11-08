Language Extension for Basic Storm
==================================

The SQL library also contains a language extension that is usable in Basic Storm. It allows writing
SQL statements directly in Basic Storm. The language extension only supports a subset of the full
range of SQL queries. On the other hand, since the language extension understands SQL, it is able to
bridge some differences between different database engines.

In addition to providing a convenient API, the language extension is also able to type-check queries
given that a declaration of the tables in the database is available. This declaration can also be
used to perform limited automatic database migrations.


Data Types
----------

To allow maximum portability, the language extension currently only supports the following data types:

- `INTEGER`/`INT`
- `REAL`
- `TEXT` (behaves like `VARCHAR`)

These data types may optionally be suffixed by a size (e.g. `TEXT(10)`) to specify their size.


Column Declarations
-------------------

The database library differs a bit from SQL in how column declarations behave. This is partially to
match what users of Basic Storm would expect, and partially to bridge differences between different
databases.

First and foremost, columns are `NOT NULL` by default. Instead, columns that may be `null` are
explicitly marked as `ALLOW NULL`. Secondly, primary keys are automatically incremented
automatically. This behavior is in line with SQLite, that binds the primary key to a rowid-field
that is automatically generated. However, it might be relevant to specify `AUTOINCREMENT` explicitly
if the application requires that the primary keys need to be monotonically increasing (this is not
guaranteed otherwise, since `AUTOINCREMENT` is not used on SQLite unless explicitly specified).

As such, the following modifiers are supported:

- `PRIMARY KEY`
- `ALLOW NULL`
- `UNIQUE`
- `AUTOINCREMENT` / `AUTO_INCREMENT`
- `DEFAULT <literal>`

It is also possible to specify multiple columns as a primary key by using `PRIMARY KEY(a, b)`.


Query Syntax
------------

The extension makes SQL queries usable as expressions in Basic Storm. Since there may be multiple
database connections active at any one time, it is, however, necessary to specify which database
connection to use. This can be done in one of two ways:

- Either by prefixing the query by `WITH <expr>:`, where `<expr>` is an expression that evaluates to
  the database connection to use.

- Or, the queries can be located (indirectly) inside a `WITH <expr> {}` block. Again, `<expr>`
  evaluates to the database connection to use.


The following queries are supported by the extension:

- `CREATE TABLE <name>(<columns>)` or `CREATE TABLE IF NOT EXISTS <name>(<columns>)`

  Creates a table. Columns are declared as described above. This expression evaluates to `void`.

- `CREATE INDEX <name> ON <table>(<columns>)` or `CREAET INDEX ON <table>(<columns>)`

  Creates an index on the specified columns. If the second form is used, a name is generated
  automatically. Evaluates to `void`.

- `DROP TABLE <table>`

  Discard a table. Evaluates to `void`.

- `INSERT INTO <table>(<columns>) VALUES (<values>)`

  Insert a row in a table. Values may be arbitrary expressions in Basic Storm. The language
  extension ensures that they are properly escaped. Evaluates to the primary key of the newly
  inserted row.

- `UPDATE <table> SET <column> = <value> WHERE <predicate>`

  Updates rows in a table. `<value>` may be an arbitrary Basic Storm expression. It is properly
  escaped. Evaluates to the number of rows updated.

- `DELETE FROM <table> WHERE <predicate>`

  Delete rows from a table. Evaluates to `void`.

- `SELECT <columns> FROM <table> WHERE <predicate>`

  Select rows from a table. `<columns>` may be `*` in which case all columns are selected. The query
  may also include joins and order by statements. Evaluates to a `Result` that can be used to
  retrieve the results. This makes it possible to use a `SELECT` statement in a for-loop.


Note that it is possible to use Basic Storm variables anywhere an SQL expression is expected (names
of tables and columns can, however, not be parameterized). The language extension takes care to use
prepared statements in these cases, so that SQL injections are not possible. It is also possible to
embed arbitrary Basic Storm expressions using `${<expr>}`, either to disambiguate variable names, or
to pre-compute values.


In addition to the standard queries mentioned above, the language extension provides a few
additional constructs for convenience:

- `SELECT ONE <columns> FROM <table> WHERE <predicate>`

  Like `SELECT`, but only returns the first row as a `Maybe<Row>`. Useful when looking up data based
  on the primary key, for example. Like `SELECT`, supports `JOIN`, and `ORDER BY` as well.

- `COUNT FROM <table> WHERE <predicate>`

  Like `SELECT COUNT(*)` in standard SQL. As such, returns the number of matching rows a `SELECT`
  statement would have returned. Supports `JOIN`, but not `ORDER BY` (as it would be meaningless).


To illustrate how it can be used, consider the following example:

```bs
use sql;

void main() {
    SQLite db(); // Create an in-memory database.

    // Create a table to work with.
    WITH db: CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT);

    // We don't want to specify WITH db every time, so we use a block:
    WITH db {
        // Insert some data.
        Str value = "test2";
        INSERT INTO test VALUES (1, "test");
        INSERT INTO test VALUES (2, value);

        // Inspect the data:
        for (row in SELECT * FROM test WHERE id < 1) {
            print(row.toS());
        }
    }
}
```


Typesafe Queries
----------------

If a table declaration is provided, it is possible to create a *typed connection* that the database
library can use to type-check SQL queries. As we shall see, this also makes it more convenient to
extract data from SQL queries.

A database declaration is declared at top-level in a file. It consists of a number of table- and
index declarations:

```bs
use sql;

DATABASE PetDB {
    TABLE person(
        id INTEGER PRIMARY KEY,
        name TEXT
    );

    TABLE pet(
        id INTEGER PRIMARY KEY,
        name TEXT,
        owner INTEGER
    );

    INDEX ON animal(owner);
}
```

This declares a database with two tables, `persons` and `pet`, and an index on the `owner` column
of `pet`. In Basic Storm, the database `PetDB` will appear as a type that represents the typed
connection. This typed connection can be used instead of a `DBConnection` when using the language
extension. It can be created as follows:

```bsstmt
SQLite untyped();     // Regular, untyped connection to an in-memory database.
PetDB typed(untyped); // Create a typed version that uses the untyped connection.
```

When the typed connection is created, it will inspect the current state of the database (using
`tables` and `schema`) and verifies that the database contains the expected tables and indices. If
this is not the case, it automatically creates any missing tables, and attempts to automatically
migrate any existing tables. The extent of the automatic migrations are, however, quite limited so
to not accidentally destroy data. For example, the migration will happily create new tables, modify
column attributes, and insert new columns that may either be `null`, or that have default values. It
will, however, not attempt to remove columns as that may cause data loss.

Due to the automatic migration above, it is not possible to use `CREATE TABLE`, `DROP TABLE`, or
`CREATE INDEX`, on a typed connection as that would violate the assumptions made by the library.

The type safety does, however, mean that the library can typecheck queries, and provide convenient
access to elements. For example, consider the following function that inserts elements in the
database:

```bs
use sql;

void insert(PetDB typed) {
    // The library knows that this is fine, since `id` is autoincrement.
    WITH typed: INSERT INTO person(name) VALUES ("Filip");

    // This is, however, not allowed since the types do not match:
    WITH typed: INSERT INTO person(name) VALUES (10);
}
```

Similarly, for select statements, the library generates custom types that represent the result rows:

```bs
use sql;

Str? findPerson(PetDB typed, Nat personId) {
    if (row = WITH typed: SELECT ONE name FROM person WHERE id == personId) {
        // Note: We access the column by name here:
        return row.name;
    } else {
        return null;
    }
}
```

This also works with joins. For example, to retrieve all pets for a particular person, one can do:

```bs
use sql;

// Note: Using "name" here is not ideal since it might not be unique,
// but it illustrates how joins work:
Array<Str> findPets(PetDB typed, Str name) {
    Array<Str> out;
    var result = WITH typed: SELECT * FROM person
        JOIN pet ON pet.owner == person.id
        WHERE person.name == name;
    for (row in result) {
        // Again, note that we access columns by name.
        out << row.pet.name;
    }
}
```

A few final remarks on named access to columns:

- The type of the column corresponds to the one declared in the table declaration. If it might be null, it is wrapped in `Maybe<T>`.
- If names contain `.`, then the entire sub-table may be `null` in the case of `LEFT`, `RIGHT` or `OUTER` joins. This
  means that it is necessary to check entire sub-tables for `null` in these cases. If column aliases (the `AS` keyword)
  are used, this might lead to excessive `null`-checking, since it is not possible to express the constraint that
  column `a` is null exactly when column `b` is null in the Storm type system without creating a nested structure.

