Url
===

The type `core.io.Url` stores a generic url. In the context of Storm, a url consists of a protocol
followed by a sequence of `/`-separated parts. The `Url` is designed to be a convenient way of
manipulating urls (such as paths) without resorting to string manipulation. As such, the `Url` class
typically does not access the file system. It is therefore possible to create `Url`s that refer to
non-existent or even invalid file names. To aid in differentiating between urls to files and urls to
paths, the `Url` class has a notion of whether it refers to a file or directory. By convention, urls
that refer to a directory are printed using a trailing slash `/`.

The associated protocol manages access to the underlying file system. As such, there are members of
the `Url` class that uses the associated protocol to check if a constructed `Url` exists, to open
it, list the contents of directories, etc. All protocols do not support all operations. For example,
the `file://` protocol that is used for local files (and pretty-printed as regular paths) supports
all operations, while the protocol for relative paths supports none of them.

The library contains a few convenience functions that produce `Url`s that serve as a starting point
for further path traversal:

- `core.io.cwdUrl` - Get an `Url` that contains the current working directory.

- `core.io.executableUrl` - Get an `Url` that refers to the path that the Storm executable was
  started from.

- `core.io.userConfigUrl(appName)` - Get an `Url` that refers to a path to a directory where it is
  suitable to store configuration for a program named `appName`.

- `core.io.httpUrl(host)` - Create an `Url` for a `http://` url for the specified host.

- `core.io.httpsUrl(host)` - Create an `Url` for a `https://` url for the specified host.


The system also contains the functions `parsePath` and `parsePathAsDir` that parses a string
containing a path into a `Url` object. These functions have the limitation that they only support
paths on the local filesystem.


An instance of the `Url` class is immutable. That is, none of the members of the class modify the
object. Instead, they return a copy of the class with the modifications applied. It has the
following operations:

- *constructor*

  Creates a new empty, relative `Url`. Relative `Url`s have to be made absolute before they can be
  accessed.

- `==`

  Compare the `Url` to another `Url`. The comparison depends on the current protocol. For example,
  on Windows paths are compared in a case insensitive manner. The operator does, however, never
  access the file system in order to resolve symbolic links, for example.

- `push` or `/`

  Appends another parth (a `Str`) or an entire relative `Url` to the end of this `Url`. Returns the
  new `Url`.

- `dir`

  Check if this `Url` refers to a directory.

- `makeDir`

  Return a copy of this `Url` that is marked as a directory.

- `name`

  Get the name of the file or directory. The name is the last part of the `Url`.

- `title`

  Get the title of the file. The title is the last part of the `Url`, without any file extension.

- `ext`

  Get the extension of the file, if any.

- `withExt`

  Create a copy of the `Url` with the specified file extension.

- `absolute`

  Check if this `Url` is absolute.

- `relative`

  The inverse of `absolute`.

- `makeAbsolute(Url base)`

  Make this `Url` into an absolute `Url` if it is not already absolute. If the current `Url` is
  relative, assume that it is relative to `base`.

- `relative(Url to)`

  Make this `Url` relative to `to`.

- `relativeIfBelow(Url to)`

  Make this `Url` relative to `to` if they share a common prefix.

- `count` and `[]`

  Access individual parts of the `url`.

- `protocol`

  Get the current protocol.

- `format`

  Format the `Url` for other C-apis. May not return a sensible string representation for all
  protocols.


Apart from the operations above, that operate on the contents of the `Url` itself, it contains the
following operations that access the underlying filesystem through the associated `Protocol`. These
operations are thus not available for all `Protocols`. In particular, they are not available for
relative urls.

- `updated`

  Return a copy of this `Url` where the `dir` member reflects the current status of the
  corresponding entry in the filesystem.

- `children`

  Return an array of `Url`s that correspond to files and directories in the current directory. The
  returned `Url`s are such that `dir` returns whether they are directories or not.

- `read`

  Open the `Url` for reading. Returns an `IStream`.

- `write`

  Open the `Url` for writing, and creates it if it does not exist. Returns an `OStream`.

- `exists`

  Check if the `Url` exists.

- `createDir`

  Create this path as a directory. Returns `false` if the parent directories do not exist.

- `createDirTree`

  Creates this path, and all parent directories.

- `delete`

  Deletes this path. Fails if the `Url` refers to a non-empty directory.

- `deleteTree`

  Deletes this path and any directories inside it.



Protocols
---------

The following protocols are supported:

- `RelativeProtocol`

  The protocol used for relative paths. Does not support any file-IO operations.

- `FileProtocol`

  Used to represent local files. Behaves according to the semantics on the current operating systems
  with regards to comparing paths.

- `HttpProtocol`

  Protocol used to represent `http` or `https` paths. The first part of the url is assumed to be the
  hostname.

- `MemoryProtocol`

  A protocol that stores a set of files in memory. Useful when a `Url` is necessary, but the program
  does not wish to store files on disk. Each instance of `MemoryProtocol` provides its own namespace
  of virtual files. It does, however, not support directories.
