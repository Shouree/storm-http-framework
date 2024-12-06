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
provides the [stormname:http.HTTP_Request] class.

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
- .__asf
```

Cookie Class
-------------
The user instantiate a `Cookie` and set each field with apropiate values. The cookie class contains a function to set cookies that is called `setCookies()`. It must be called by the user to add the cookie to the `HTTP_Response`.

The Cookie class contains the following fields that can be set by the user:
```
  Str name;
  Str value;
  Str path = ""; 
  Str domain ="";
  Str expires ="";
  Nat maxAge = 0;
  Bool secure = false;
  Bool httpOnly = false;
  Str samSite ="";
  Bool cookieValid = true;
```

Server Class
-------------

The server class [stormname:http.HTTP_Server] contains functions to receive `HTTP_Request`
and send `HTTP_Response` through a given port. The server can handle Keep-alive connection that times out after 60 seconds of no data being sent. This timeout can be set manualy by the user when initiating the server, using the function `setTimeout()`.
```
HTTP_Server server(1234);
server.setTimeout(5 s);
```
The server is multithreaded and can handle multiple clients in parallel. When a client sends a request to the server, on the specified port, the server will create a new thread for the connection to the client.

To run the server, run the following

```
HTTP_Server server(1234);

  server.addCallback(
    (HTTP_Request req) =>{
    HTTP_Response res;
    // Add functionality here
    return res;
    }
  );

while(true)
    server.recieve();
```

In the default callback, you need to construct a function that generates and returns a `HTTP_Response` which will be sent to the client

#### Routing


 

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
