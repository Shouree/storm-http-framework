Standard Streams
================

The standard library provides access to the streams that are attached to standard input, standard
output, and standard error in the operating system. They are present in the package `core.io.std`,
and are named:

- [stormname:core.io.std.in] - Standard input stream.
- [stormname:core.io.std.out] - Standard output stream.
- [stormname:core.io.std.error] - Standard error stream.


For text input or output, the system also provides versions of the above-mentioned three streams in
the `core` package:

- [stormname:core.stdIn] - Standard input, as a text stream.
- [stormname:core.stdOut] - Standard output, as a text stream.
- [stormname:core.stdError] - Standard error, as a text stream.

The text version of the streams can be modified and set to any stream. This has the effect of
redirecting any output from the system to the supplied stream. The system also provides a generic
`print` function that prints to `core.stdOut`:

- [stormname:core.print(Str)] - Print a string to standard output.
