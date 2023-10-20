IO
==

The standard library provides the package `core.io` that deals with input and output. The library is
centered around the idea of input- and output- streams, and provides streams for accessing local
files, and connect to the network through sockets. The library also contains the `Url` class that is
able to manipulate paths and urls in a convenient way.

As a layer on top of streams, the library also provides mechanism to read binary data and convert it
to text, as well as generic serialization of objects in the system.

