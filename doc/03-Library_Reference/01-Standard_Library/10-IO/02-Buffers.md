Buffers
=======

The type `core.io.Buffer` is used to store and transfer binary data in the IO library. In
particular, it plays a central role when reading from and writing to streams. In addition to this,
it can also be used in other contexts where it is necessary to store binary data without attaching a
particular meaning to any of the bytes.

A `core.io.Buffer` is a value and is therefore handled by value. To avoid repeatedly copying large
amounts of data, the `Buffer` implements by-reference semantics internally. This means that making a
copy of a `Buffer` object creates two instances that refer to the same data, just as a class type
would behave. Data is, however, copied whenever a buffer is passed between threads in order to avoid
shared data. Because of these properties, it is typically a good idea to design interfaces that
operate on `Buffer`s to accept a buffer, and return a modified copy of the buffer. If the interface
internally operates on and returns the same buffer, no copies will be made. The interface will,
however, still function correctly if the system needs to copy data at any point. The `IStream` and
`OStream` classes are designed according to this idea.

Each `Buffer` instance stores three things: a buffer large enough for a pre-determined number of
bytes, the size of said buffer, and how many bytes are considered used. The number of bytes is
ignored by the `Buffer` itself in most cases, and is rather designed as a convenient ways for other
parts of the system to communicate how much of the buffer was actually filled.


The `Buffer` type has the following members. Note that some of them are implemented as free
functions due to technicalities in the interface between Storm and C++. In languages like Basic
Storm it is, however, possible to treat them as if they were members anyway.


Member Functions
----------------

- *constructor*

  Create an empty buffer. Use the free function `buffer` to create a buffer with a specific size.

- `buffer(Nat size)`

  Create a buffer with the specified size. The buffer is initially empty (i.e. `filled` is set to zero).

- `count()`

  Return the number of bytes in the buffer.

- `filled()`, `filled(Nat)`

  Return or set the number of used bytes in the buffer. The set-function is marked as an assignment
  functions, which means that it is possible to treat this member as a regular member variable in
  languages like Basic Storm.

- `[]`

  Access the byte at the specified index. Returns a reference, so it is possible to modify the byte
  as well as reading it.

- `empty()`

  Returns `true` if the buffer is empty. A buffer is empty when `filled` is zero.

- `full()`

  Returns `true` if the buffer is full. A buffer is full when `filled` is equal to `count`.

- `cut(Nat from)` and `cut(Nat from, Nat to)`

  Analogous to the `cut` functions in the `Str` class: creates a copy of the bytes from `from`
  until, but not including, `to` (or the end if `to` is not specified).

- `grow(Nat newCount)`

  Return a new buffer with a different size. Copies the data to the new buffer as well.

- `push(Byte)`

  Insert a single byte of data to the end of the buffer, as denoted by `filled`, and increase
  `filled` by one. Returns `false` if the buffer was full.

- `shift(Nat n)`

  Discard the first `n` bytes, and move the remaining bytes towards the start of the buffer.
  `filled` is updated to reflect this operation. As such, this operation essentially discards the
  first `n` bytes in the buffer while retaining any remaining bytes. Note that this operation
  requires copying the contents of the buffer, and should thus be used sparingly. For example,
  continuously reading a single byte and shifting the buffer will result in poor performance.


Free Functions
--------------

- `<<` and `toS`

  Creates a string representation of the buffer. As a buffer contains a sequence of bytes, it is
  outputted as a hex dump. The location of `filled` is marked with a pipe (`|`). Buffers are thus a
  convenient way to inspect binary data in textual form.

- `outputMark(StrBuf, Buffer, Nat)`

  As `<<` and `toS`, but allows specifying an additional mark in the output. The mark is drawn with
  a `>` character.

- `toUtf8(Str)`

  Create a buffer with the contents of a `Str` encoded as UTF-8. [Text streams](md:Text_IO) can also
  achieve this.

- `fromUtf8(Buffer)`

  Create a `Str` from the contents of a buffer. [Text streams](md:Text_IO) can also achieve this.
