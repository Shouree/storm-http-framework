Networking
==========

The package `core.net` contains classes to communicate through a network. To allow this, the library
provides a representation of IP addresses, sockets, and listener sockets. The network interfaces
generally throw the [stormname:core.net.NetError] when an error occurs.


Address
-------

The [stormname:core.net.Address] class is an abstract representation of an *internet address* (e.g.
IPv4 or IPv6 addresses), and an associated port. If the port is zero, then the port is unspecified,
and will be allocated automatically by the system. The `Address` class is immutable, which means
that it is not possible to modify an instance that has been created. Instead, it is necessary to
create a copy with the desired modifications.

Subclassing the `Address` class requires low-level access to the memory representation used by the
operating system. Therefore, it is not possible to create new derived classes from Storm. For this
reason, the `Address` class has no public constructors exposed to Storm.

`Address` classes are typically created by calling the function
[stormname:core.net.lookupAddress(Str)] to parse a string into an address. This interface supports
DNS queries as well.

The `Address` class has the following members in additional to the usual `toS` and `hash` members:

```stormdoc
@core.net.Address
- .port()
- .withPort(*)
```

The system currently provides two subclasses, one for IPv4 addresses, and another for IPv6
addresses. The IPv4 class, [stormname:core.net.Inet4Address], has the following additional members:

```stormdoc
@core.net.Inet4Address
- .data()
- .count()
- .[](*)
```

For IPv6 addresses, the class [stormname:core.net.Inet6Address] is used. It has the following
additional members:

```stormdoc
@core.net.Inet6Address
- .count()
- .[](*)
- .flowInfo()
- .scope()
```


Sockets
-------

Generic sockets are represented by the class [stormname:core.net.Socket]. This class provides
generic socket control operations that are relevant both for TCP and UDP. Currently, only TCP
sockets are supported. A TCP socket is represented by the class [stormname:core.net.NetStream].

A client socket is created by calling one of the `connect` functions:

```stormdoc
- core.net.connect(core.net.Address)
- core.net.connect(core.Str, core.Nat)
```

A `NetStream` contains the following members:

```stormdoc
@core.net.NetStream
- .input()
- .output()
- .nodelay(*)
- .remote()
- .close()
```

Closing the input and/or output streams shuts down that part of the TCP stream. The socket itself
must also be closed for a proper shutdown. It is possible to simply close the socket without closing
either of the input and output streams. Some protocols do, however, require graceful shutdown of one
or both ends.

The generic `Socket` interface also contains the following properties that can be get and set using
getters and setters: `inputTimeout`, `outputTimeout`, `inputBufferSize`, and `outputBufferSize`.


Listener
--------

A [stormname:core.net.Listener] represents an open TCP socket that accepts incoming connections. A
`Listener` is created by calling one of the `listen` functions as follows. The socket option
`SO_REUSEADDR` is set by default so that it is possible to reuse a port immediately, even after a
previous process failed to shut down properly. This can be overridden by specifying `reuseAddr` to
`false` in the calls to the `listen` functions below:

```stormdoc
- core.net.listen(*)
```

The `Listener` class inherits from [stormname:core.net.Socket], and adds the function:

```stormdoc
- core.net.Listener.accept()
- core.net.Socket.close()
```

As with all sockets, the listener should be closed when it is no longer needed.
