Buffers
=======

The type [stormname:core.io.Buffer] is used to store and transfer binary data in the IO library. In
particular, it plays a central role when reading from and writing to streams. In addition to this,
it can also be used in other contexts where it is necessary to store binary data without attaching a
particular meaning to any of the bytes.

A [stormname:core.io.Buffer] is a value and is therefore handled by value. To avoid repeatedly
copying large amounts of data, the `Buffer` implements by-reference semantics internally. This means
that making a copy of a `Buffer` object creates two instances that refer to the same data, just as a
class type would behave. Data is, however, copied whenever a buffer is passed between threads in
order to avoid shared data. Because of these properties, it is typically a good idea to design
interfaces that operate on `Buffer`s to accept a buffer, and return a modified copy of the buffer.
If the interface internally operates on and returns the same buffer, no copies will be made. The
interface will, however, still function correctly if the system needs to copy data at any point. The
`IStream` and `OStream` classes are designed according to this idea.

Each `Buffer` instance stores three things: a buffer large enough for a pre-determined number of
bytes, the size of said buffer, and how many bytes are considered used. The number of bytes is
ignored by the `Buffer` itself in most cases, and is rather designed as a convenient ways for other
parts of the system to communicate how much of the buffer was actually filled.


The `Buffer` type has the following members. Note that some of them are implemented as free
functions due to technicalities in the interface between Storm and C++. In languages like Basic
Storm it is, however, possible to treat them as if they were members anyway.


Member Functions
----------------

```stormdoc
@core.io.Buffer
- .__init()
- .count()
- .filled()
- .filled(Nat)
- .free()
- .[](*)
- .empty()
- .full()
- .push(*)
- .shift(*)
```

Free Functions
--------------

The following functions are implemented outside of `Buffer` for technical reasons. Most of them can
be considered proper member functions of the `Buffer` type.

```stormdoc
@core.io
- .buffer(Nat)
- .cut(core.io.Buffer, Nat)
- .cut(core.io.Buffer, Nat, Nat)
- .grow(core.io.Buffer, Nat)
- .<<(core.StrBuf, core.io.Buffer)
- .outputMark(core.StrBuf, core.io.Buffer, Nat)
- .toUtf8(core.Str)
- .fromUtf8(core.io.Buffer)
```
