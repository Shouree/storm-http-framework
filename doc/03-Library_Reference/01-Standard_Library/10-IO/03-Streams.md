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

There are three types of streams in Storm: output streams, input streams, and random-access input
streams.


Output Streams
--------------

```stormdoc
core.io.OStream
- .write(*)
- .flush()
- .close()
```

[stormname:core.Array(core.Int)]

Output streams accept data that is written to some destination. They are implemented as subclasses
to the class `core.io.OStream`. Output streams are not buffered in general, so it is desirable to
write data in chunks whenever possible. The class `core.io.BufferedOStream` can be used to wrap an
output stream to ensure data is buffered.

The `OStream` class has the following members:

- `write(Buffer b)`

  Write data from the start of the supplied buffer until the `filled` marked in the buffer. Will
  block until all data has been written.

- `write(Buffer b, Nat start)`

  Like `write`, but writes data from `start` until `filled`.

- `flush()`

  Flush any buffered data to the destination. The exact behavior of this operation depends on the
  stream that is used. For example, file- and network streams are generally unbuffered by default,
  so it is not necessary to flush them. There are, however, streams that buffer the input for
  efficiency (e.g. `core.io.BufferedOStream`). These use `flush()` to allow manually flushing the
  buffer. Calls to `flush` are generally passed along chains of output streams. For example, calling
  `flush` on a `BufferedOStream` will flush the `BufferedOStream` and call flush on whichever stream
  the `BufferedOStream` is writing its data to.

- `close()`

  Closes the stream and frees any associated resources. As with `flush`, this is generally
  propagated in the case of chained streams.

  The destructor of the `OStream` calls close as a last resort. Since output streams are garbage
  collected, it might be a while before the destructor is called. As such, manually closing streams
  is generally preferred.


Input Streams
-------------

An input stream reads data from some source. They are implemented as subclasses to the class
`core.io.IStream`. Input streams are generally not buffered, so it is desirable to read data in
chunks.

The `IStream` class has the following members:

- `more()`

  Returns `false` if the end of the stream has been reached, and `true` otherwise. In some cases it
  is not possible to determine the end of stream ahead of time. In such cases, `more` might return
  `true` even if `read` will later report that it managed to read zero bytes. Whenever `read`
  succeeds with zero bytes, `more` will start returning `false` afterwards.

- `read(Buffer to)`

  Read data from the stream and attempt to fill the bytes from `filled` to `count` in the buffer
  `to` with bytes from the stream. Returns a buffer with the newly acquired data, and a `filled`
  member that has been updated to reflect the number of read bytes. Generally, the returned buffer
  refers to the same contents as `to` to avoid copying potentially large amounts of data. The
  implementation is, however, free to re-allocate the buffer as needed. In particular, this is
  always the case when the system needs to execute code on another thread, and thereby copy the
  buffers. Because of this, users of the `read` function shall always use the returned buffer and
  not rely on the buffer passed as `to` being updated.


  The function blocks until at least one byte has been successfully read from the stream, or if the
  end of stream has been reached. This means that if `filled` has not been updated from its original
  location, then it is guaranteed that the end of the stream has been reached. Note, however, that
  the `read` function does not guarantee that the entire buffer has been filled. This often happens
  in the case of networking, for example. If the remote end has only sent one byte, a read operation
  of 10 bytes will typically only result in 1 out of 10 bytes being returned, since filling the
  buffer would potentially block foreveer (for example, the remote end might wait for a response).

- `read(Nat maxBytes)`

  Like `read`, but allocates a new buffer rather than reusing an existing one.

- `fill(Buffer to)`

  Like `read`, but repeats the read operation until the buffer is full, or until the end of the
  stream has been reached. This means that if the buffer is not full after `read` has been called,
  then the end of stream has always been reached. Do, however, note that this behavior may not
  always be desirable when working with pipes or networking.

- `fill(Nat bytes)`

  Like `fill`, but allocates a new buffer rather than reusing an existing one.

- `peek(Buffer to)`

  Peek bytes from the stream and attempt to fill `to` similarly to `read`. The difference is that
  the bytes returned will not be consumed, and can be read again using a `read` or another `peek`
  operation.

  The exact number of bytes that can be safely acquired through a `peek` operation depends on the
  stream in use.

- `peek(Nat maxBytes)`

  Like `peek`, but allocates a new buffer rather than reusing an existing one.

- `close()`

  Closes the stream and frees any associated resources. As with `flush`, this is generally
  propagated in the case of chained streams.

  The destructor of the `OStream` calls close as a last resort. Since output streams are garbage
  collected, it might be a while before the destructor is called. As such, manually closing streams
  is generally preferred.

- `randomAccess()`

  Create a random access stream based on this stream. For streams that are random access, this
  function simply returns the same stream, but casted to a random access stream. For streams that do
  not natively support random access, it instead a `core.io.LazyMemIStream` that buffers data to
  allow seeking from this point onwards. Regardless of which case was used, the original stream does
  not need to be closed.


Random-Access Input Streams
---------------------------

Random-access input stream are derived from `core.io.RIStream`. The class `RIStream` inherit from
`IStream`, which means that random-access input streams are usable as regular input streams. Random
streams provide the following members in addition to regular streams:

- `length`

  Get the length of the input data, in bytes.

- `tell()`

  Return the current position in the stream, as a `Word`.

- `seek(Word pos)`

  Seek in the stream, always relative to the start of the stream. This makes the next `read`,
  `peek`, or `fill` operation read from the specified position.
