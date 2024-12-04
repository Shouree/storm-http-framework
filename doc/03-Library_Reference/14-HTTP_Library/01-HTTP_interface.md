HTTP Interface
==================

The HTTP library 

Basic Types
-------------

The HTTP library does not presume that it delivers all functions that a user of the library could possibly need.
Therefore, two basic data types are available with public data:

- [stormname:http.HTTP_Request] : Contains 
- [stormname:http.HTTP_Response]
These data types make use of three enums:
- [stormname:http.HTTP_Version]
- [stormname:http.HTTP_StatusCode]
- [stormname:http.HTTP_Method]

To help users of the library account for differences between different databases, the library
provides the [stormname:http.HTTP_Request] class. The class can be thought of as a regular string, but
some parts of the string have semantic information attached to them. For example, the `QueryStr`
class knows that a certain part is supposed to be a placeholder, a name, or a type.

```stormdoc
@http.HTTP_Request
- .__dfhdfh
- .__asfasfasf
- .__asf
```
```stormdoc
@http.HTTP_Response
- .__dfhdfh
- .__asfasfasf
- .__asf```



Server Class
------------

The server class [stormname:http.HTTP_Server] is contains functions to receive `HTTP_Request`
and send `HTTP_Response` through a given port.
```stormdoc
@http.HTTP_Server
- .__init(Int port)
- .__recieve()
- .__send(HTTP_Response)
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
