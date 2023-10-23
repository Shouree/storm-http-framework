Text IO
=======

Storm needs to read text files from the file system to parse source code among other tasks. As such,
the standard library contains fairly competent facilities for reading and writing text files in
various encodings.

As mentioned in the documentation for the [string class](md:../Strings), Storm internally works with
unicode codepoints in UTF-32 (of course, the internal representation is more compact). For this
reason, the task of the text IO system is to convert encodings to and from UTF-32 in order to read
and write them to streams.

When reading a stream (e.g. a file), the encoding is determined by the first few bytes of the file.
Storm assumes that text files are encoded in some form of unicode, unless instructed otherwise. If a
Byte Order Mark (BOM) is found as the first character in the file, the encoding of the BOM
determines the encoding of the entire file (this is commonplace in Windows). If no BOM is present,
the system assumes that the file is encoded in UTF-8.

A BOM is the codepoint `U+FEFF`. If encoded in UTF-8, it will be encoded as `EF BB BF`. In UTF-16,
it is encoded as either `FE FF` or `FF FE` depending on whether big- or little- endian encoding was
used. In the standard decoding process the BOM is silently consumed before the decoded text is
passed on further. The users of the library will therefore typically never see any BOMs in the file.

The text library also handles differences in line endings between different systems. The text
decoding process converts line endings into `\n`, and the output process converts them as
appropriate for the system.


Text Information
----------------

The class [stormname:core.io.TextInfo] stores information about encoded text. It is used to specify
how the text output system should behave when encoding text:

```stormdoc
@core.io.TextInfo
- .__init(*)
- .useCrLf
- .useBom
```

There are also functions that create `TextInfo` instances that describe the default behavior on
various systems:

```stormdoc
@core.io
- .sysTextInfo()
- .unixTextInfo()
- .windowsTextInfo(*)
```

Text Input
----------

The core interface for reading text from a stream is [stormname:core.io.TextInput]. Derived classes
override the abstract function `readChar` that the rest of the class uses to read single characters
and compose them into higher-level read operations. As mentioned above, this includes hiding the
byte order mark if required, converting line endings, etc.

Subclasses generally take a stream as an input source, and they implement their own buffering to
avoid excessive calls to `IStream.read()`.

The [stormname:core.io.TextInput] class has the following members:

```stormdoc
@core.io.TextInput
- .more()
- .read()
- .peek()
- .readLine()
- .readAll()
- .readRaw()
- .readAllRaw()
- .close()
```

There are also a number of convenience functions that creates `TextInput` instances, and reads text:

```stormdoc
@core.io
- .readText(IStream)
- .readText(Url)
- .readStr(Str)
- .readAllText(Url)
```

Text Output
-----------

Text output is handled by the interface [stormname:core.io.TextOutput]. Output objects must
generally be created manually to select the output encoding. The next section provides an overview
of supported encodings. Creating the output object typically requires specifying a
[stormname:core.io.TextInfo] object that determines how line endings should be handled, and if a BOM
should be emitted or not. Output streams generally buffer output to avoid many small writes to the
underlying streams. As such, applications that need control over when data is written to the stream
(e.g. networking) may need to call `flush` explicitly.

The text output interface has the following members:

```stormdoc
@core.io.TextOutput
- .__init()
- .__init(TextInfo)
- .autoFlush
- .write(*)
- .writeLine(*)
- .flush()
- .close()
```


Text Encodings
--------------

The following text encodings are supported by the system:

```stormdoc
- core.io.Utf8Input
- core.io.Utf16Input
- core.io.Utf8Output
- core.io.Utf16Output
```