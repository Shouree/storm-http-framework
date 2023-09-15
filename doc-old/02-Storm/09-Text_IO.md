Text IO
========

Storm generally reads input from text files in the file system. Storm provides tools for reading and
converting text files to the internal character representation (currently UTF-16). Most languages
will this standard mechanism to read their input, and will therefore accept text files as described
here.

When reading a file (or a stream), the encoding is determined based on the first few bytes of the
file. If a Byte Order Mark (BOM) is found as the first character of the file, the encoding of the
BOM is assumed to represent the encoding of the entire file. If the BOM is not found, the file is
assumed to be encoded in UTF-8 (even if your system codepage is something different). Currently,
UTF-8 and UTF-16 (both little and big endian) are supported.

A BOM is the UTF codepoint `U+FEFF`. If encoded in UTF-8, it will be encoded into the following
bytes: `EF BB BF`, UTF-16 uses either `FE FF` or `FF FE` based on endianness. In the standard
decoding process, the BOM is silently consumed before the decoded text is passed on further, so the
users of Storm's standard text input will not notice the presence or absence of a BOM.

The IO library
---------------

The IO library is located in the `core.io` package, and is based on streams. A stream is a raw
byte-based data stream in either direction. Streams are implemented by `IStream` and `OStream` for
input and output, respectively.

To read text, the `TextReader` and `TextWriter` classes can be used. These read binary data from a
stream and converts it from the detected character set into UTF-32 characters or `Str`. To
auto-detect the input encoding from a stream and create a `TextReader` based on the result, use the
function `readText`. Currently there is nothing equivalent for output streams, but it will be
possible to copy the format from a `TextReader` to get the same output format.

Another central part of the IO library is the `Url` class. An `Url` represents a file or resource
somewhere. In Storm, an `Url` is a protocol, followed by a list of strings that makes up the path
itself. The `Url` class keeps track if it is referring to a directory or a file, and indicates this
by outputting directory names with a trailing `/`. The protocol is a class that tells the `Url` how
to access files for paths relative to that protocol. Currently, Storm only implements the `file://`
protocol. If an `Url` does not have a protocol, it is assumed to represent a relative path. Relative
paths can not be accessed directly, but must first be appended to some base `Url`. To create a `Url`
from a string representing a path on your local machine, use the `parsePath` function.
