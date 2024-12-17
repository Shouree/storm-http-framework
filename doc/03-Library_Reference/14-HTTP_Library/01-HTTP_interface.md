HTTP Interface
==================

The HTTP library 

Basic Types
-------------

The HTTP library does not presume that it delivers all functions that a user of the library could possibly need.
Therefore, two basic data types are available with public data:

- [stormname:http.HTTP_Request] 
```bs
  HTTP_StatusCode imediate_response;
  HTTP_Method method;
  HTTP_Version version;
  Url path;
  Str->Str method_params;
  Str->Str headers;
  Buffer data;
  Str->Str cookies;
```
- [stormname:http.HTTP_Response]
```bs
  HTTP_Version version;
  HTTP_StatusCode status_code;
  Str->Str headers;
  Buffer data;
  Array<Cookie> cookies;
```
These data types make use of three enums:
- [stormname:http.HTTP_Version]
```bs
enum HTTP_Version{
  HTTP_0_9,
  HTTP_1_0,
  HTTP_1_1
}
```
- [stormname:http.HTTP_StatusCode]
```bs
enum HTTP_StatusCode{
  NO_ERROR = 0,
  //Informational 1xx
  Continue = 100,
  Switching_Protocol = 101,
  
  //Successful 2xx
  OK = 200,
  Created = 201,
  Accepted = 202,
  Non_Authorative_Information = 203,
  No_Content = 204,
  Reset_Content = 205,
  Partial_Content = 206,
  
  //Redirectional 3xx
  Multiple_Choice = 300,
  Moved_Permanently = 301,
  Found = 302,
  See_Other = 303,
  Not_Modified = 304,
  Use_Proxy = 305,
  //306 (Unused) Används inte längre
  Temporary_Redirect = 307,
  
  //Client Error 4xx
  Bad_Request = 400,
  Unauthorized = 401,
  Payment_Required = 402,
  Forbidden = 403,
  Not_Found = 404,
  Method_Not_Allowed = 405,
  Not_Acceptable = 406,
  Proxy_Authentication_Required = 407,
  Request_Timeout = 408,
  Conflict = 409,
  Gone = 410,
  Length_Required = 411,
  Prediction_Failed = 412,
  Request_Entity_Too_Large = 413,
  Request_URI_Too_Long = 414,
  Unsupported_Media_Type = 415,
  Request_Range_Not_Satisfiable = 416,
  Expectation_Failed = 417,
  
  //Server Error 5xx
  Internal_Server_Error = 500,
  Not_Implemented = 501,
  Bad_Gateway = 502,
  Service_Unavailable = 503,
  Gateway_Timeout = 504,
  HTTP_Version_Not_Supported = 505
}
```
- [stormname:http.HTTP_Method]
```bs
enum HTTP_Method{
  OPTIONS,
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  TRACE,
  CONNECT,
  BAD_METHOD
}
```


Cookie Class
-------------
The user instantiate a `Cookie` and set each field with appropriate values. The cookie class contains a function to set cookies that is called `setCookies()`. It must be called by the user to add the cookie to the `HTTP_Response`.

The Cookie class contains the following fields that can be set by the user:
```bs
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
and send `HTTP_Response` through a given port. The server can handle Keep-alive connection that times out after 60 seconds of no data being sent. This timeout can be set manually by the user when initiating the server, using the function `setTimeout()`.
```bs
HTTP_Server server(1234);
server.setTimeout(5 s);
```
The server is multithreaded and can handle multiple clients in parallel. When a client sends a request to the server, on the specified port, the server will create a new thread for the connection to the client.

To run the server, run the following

```bs
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

As an example, one can use the http library to make a simple server:

```bs

use core:net;
use core:io;
use http;
use core:lang;
use lang:bs:macro;

void main(){
  Url root_dir = cwdUrl() / "site";
  HTTP_Server server(1234);
  server.setTimeout(5 s);
  
  //Add default callback
  server.addCallback(
    (HTTP_Request req) =>{
    HTTP_Response res;
    res.version = HTTP_Version:HTTP_1_1;

    res.status_code = HTTP_StatusCode:OK;

    res.headers.put("content-type", "text/html; charset=utf-8");
    Url index_page = root_dir / "index.html";
    print("\nSending response...");
    Str data = index_page.readAllText.toS();
    res.data = toUtf8(data);
    return res;
    }
  );

  //Add callback to /chat/**wildcard**/hello
  server.addCallback(Url(["chat", "*", "hello"]),
    (HTTP_Request req) =>{
    HTTP_Response res;
    res.version = HTTP_Version:HTTP_1_1;

    res.status_code = HTTP_StatusCode:OK;

    res.headers.put("content-type", "text/html; charset=utf-8");
    Url index_page = root_dir / "chat" / "index.html";
    print("\nSending response...");
    Str data = index_page.readAllText.toS();
    res.data = toUtf8(data);
    return res;
    }
  );
  
  while(true)
    server.recieve();
}

```
