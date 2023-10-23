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

## Creating Urls

The library contains a few convenience functions that produce `Url`s that serve as a starting point
for further path traversal:

```stormdoc
@core.io
- .cwdUrl()
- .executableUrl()
- .userConfigUrl(*)
- .httpUrl(*)
- .httpsUrl(*)
```

The system also contains the functions `parsePath` and `parsePathAsDir` that parses a string
containing a path into a `Url` object. These functions have the limitation that they only support
paths on the local filesystem:

```stormdoc
@core.io
- .parsePath(*)
- .parsePathAsDir(*)
```

An instance of the `Url` class is immutable. That is, none of the members of the class modify the
object. Instead, they return a copy of the class with the modifications applied. It has the
following operations:


```stormdoc
@core.io.Url
- .__init(*)
- .==(*)
- ./(*)
- .push(*)
- .pushDir(*)
- .dir()
- .makeDir()
- .name()
- .title()
- .ext()
- .withExt(*)
- .absolute()
- .relative()
- .makeAbsolute(*)
- .relative(*)
- .relativeIfBelow(*)
- .count()
- .empty()
- .any()
- .[](*)
- .protocol
- .format()
```

Apart from the operations above, that operate on the contents of the `Url` itself, it contains the
following operations that access the underlying filesystem through the associated `Protocol`. These
operations are thus not available for all `Protocols`. In particular, they are not available for
relative urls.

```stormdoc
@core.io.Url
- .updated()
- .children()
- .read()
- .write()
- .exists()
- .createDir()
- .createDirTree()
- .delete()
- .deleteTree()
```

Protocols
---------

The following protocols are provided by the library:

- [stormname:core.io.RelativeProtocol]

  The protocol used for relative paths. Does not support any file-IO operations.

- [stormname:core.io.FileProtocol]

  Used to represent local files. Behaves according to the semantics on the current operating systems
  with regards to comparing paths.

- [stormname:core.io.HttpProtocol]

  Protocol used to represent `http` or `https` paths. The first part of the url is assumed to be the
  hostname.

- [stormname:core.io.MemoryProtocol]

  A protocol that stores a set of files in memory. Useful when a `Url` is necessary, but the program
  does not wish to store files on disk. Each instance of `MemoryProtocol` provides its own namespace
  of virtual files. It does, however, not support directories.
