Streams
=======

A *stream* represents a stream of bytes, either being read from some source, or written to some
destination. Storm provides different streams for different sources and destinations. These all
present the same interface, which means that programs typically do not need to be aware of the exact
source or destination of the data.

All streams in Storm present a synchronous interface. Utilizing the cheap [user level
threads](md:/Language_Reference/Storm/Threading_Model) in Storm, it is possible to create
nonblocking calls by simply spawning a new user level thread (`spawn` in Basic Storm). Using
multiple user level threads in this fashion makes Storm issue nonblocking IO requests to the
operating system, so that other user level threads are not blocked by long-running IO requests (this
is not the case for disk access on Linux currently due to the POSIX API, but it will be fixed
eventually).

There are three types of streams in Storm: output streams, input streams, and random access input
streams.


Output Streams
--------------

```stormdoc
!core.io.OStream
```

The `OStream` class has the following members:

```stormdoc
@core.io.OStream
- .write(*)
- .flush()
- .close()
```


Input Streams
-------------

```stormdoc
!core.io.IStream
```

The `IStream` class has the following members:

```stormdoc
@core.io.IStream
- .more()
- .read(*)
- .fill(*)
- .peek(*)
- .close()
- .randomAccess()
```


Random Access Input Streams
---------------------------

Random access input stream are derived from `core.io.RIStream`. The class `RIStream` inherit from
`IStream`, which means that random-access input streams are usable as regular input streams. Random
streams provide the following members in addition to regular streams:

```stormdoc
@core.io.RIStream
- .length()
- .tell()
- .seek(Word)
```
