SQL
====

The package `sql` contains an experimental generic database interface. It currently only supports
SQLite databases, but allows additional database connections to be added in the future by creating
other subclassess to the `DBConnection` and `Statement` classes.

The SQL extension was originally created by: David Ångström, Markus Larsson, Mohammed Hamade, Oscar
Falk, Robin Gustavsson, Saima Akhter

Using the generic interface allows connecting and using a database as follows:

```
void useDB() {
    SQLite db("file.db");

    var statement = db.prepare("SELECT * FROM test WHERE id > ?;");
    statement.bind(0, 18);
    for (row in statement) {
        print(row.getStr(1));
    }
}
```

One can accomplish the above a bit easier by using the Basic Storm extension:

```
use sql;

void useDB() {
    SQLite db("file.db");
    WITH db: CREATE TABLE test(id INTEGER PRIMARY KEY, name TEXT);

    WITH db {
        Str value = "test2";
        INSERT INTO test VALUES (1, "test");
        INSERT INTO test VALUES (1, value);

        for (row in SELECT * FROM test WHERE id > 18) {
            print(row.getStr(1));
        }
    }
}
```

As can be seen in the above example, the `WITH` keyword is used to associate statements with a SQL
statement. This statement has two forms, the first associated with a single statement (as with the
`CREATE TABLE` above), and the second form is a block that allows multiple statements inside without
specifying the database connection at each step.

The above example does not allow Storm to type-check any of the data. For that, we need to use a
typed connection. A typed connection is associated with a database declaration, that allows Storm to
type check all statements ahead of time. This also delegates the creation of tables to Storm.

This is done as follows:

```
use sql;

DATABASE MyDB {
    TABLE test(
        id INTEGER PRIMARY KEY,
        name TEXT
    );

    INDEX ON test(name);
}

void useDB() {
    SQLite db("file.db");
    MyDB typed(db);

    WITH typed {
        Str value = "test2";
        INSERT INTO test VALUES (1, "test");
        INSERT INTO test VALUES (1, value);

        for (row in SELECT * FROM test WHERE id > 18) {
            print(row.name);
        }
    }
}
```

In this case, we can see some relevant differences. First, we declare the structure of our database
in a `DATABASE` block. In this case we only declare one table and one index there, but any number of
tables and indices are possible. This declaration becomes a type in the name tree, and is used to
create a typed connection inside `useDB`.

The typed nature of the connection means that the system ensures that the appropriate tables exist
when the `MyDB` instance is created. It throws an exception if something is awry. The implementation
allows additional tables to be present, and any missing tables and indices are created.

As can be seen in the `for`-loop, the typed nature of the connection also allows accessing the
fields by name (`print(row.name)`). In case names from the SQL query contains dots (for example, in
case of `JOIN`s), they are replaced by underscores. The SQL `AS` keyword can be used to name the
outputs.

Finally, a typed nature type-checks inserts and the expressions in any `WHERE` statements according
to the table declarations. For example, the system will warn if a column needs a value during
insert.

*Note:* Columns declared using the syntax extension are `NOT NULL` by default. Use `ALLOW NULL` if a
 null value is desired. This also means that any values from `SELECT` statements will have the type
 `Maybe<T>` as appropriate.

The following example illustrates a more complex query:

```
DATABASE TwoTables {
    TABLE test(
        id INTEGER PRIMARY KEY,
        name TEXT,
        city TEXT DEFAULT "none"
    );

    TABLE extra(
        id INTEGER PRIMARY KEY,
        test INTEGER,
        data TEXT
    );
}

void useDB() {
    SQLite db; // in-memory database
    TwoTables c(db);

    WITH c {
        INSERT INTO test VALUES (3, "a", "a");
        INSERT INTO test VALUES (4, "b", "b");

        INSERT INTO extra(test, data) VALUES (3, "more");
        INSERT INTO extra(test, data) VALUES (3, "even more");
        INSERT INTO extra(test, data) VALUES (4, "some more");

        for (row in SELECT test.id AS id, data FROM test JOIN extra ON extra.test == test.id WHERE test.id == 3) {
            print(row.id # " " # row.data);
        }

        for (row in SELECT * FROM test JOIN extra ON extra.test == test.id WHERE test.id == 4) {
            print(row.test.id # " " # row.extra.data);
        }

        // Special case when you only want one row if it exists. Returns a Maybe-type instead of an iterator.
        if (x = SELECT ONE * FROM test JOIN extra ON extra.text == test.id WHERE test.id == 4) {
            print(row.test.id # " " # row.extra.data);
        }

        // We can also count the number of rows from a query (similar to SELECT COUNT(*) FROM...)
        Nat rows = COUNT FROM extra WHERE test == 3;
        print("Number of rows where test == 3: ${rows}"):
    }

}
```

*Note:* The syntax `${<expr>}` can be used to embed more complex Basic Storm expressions inside the
 query, including differentiating between variables in the surrounding context and in SQL.

*Note:* This library is fairly early in the development phase. As such, some aspects may change in
the future. In particular, how types between Storm and SQL is one thing that will likely be improved
as support for other databases are added.
