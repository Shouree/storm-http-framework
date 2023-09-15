Languages
===========

Languages in Storm are identified by the file extensions of the files containing the language. When
Storm tries to load the contents of a package, it examines the different file extensions present and
creates an instance of the `core.lang.PkgReader` class for each of the files. This is done by
calling the function `lang.<ext>.reader(core.Array<core.io.Url>, core.lang.Package)`, which is
expected to create a reader which can be used to read the files provided into the package
provided. When all `Readers` are created, Storm asks each of them to load various parts of the
language. This is to allow the different languages to find types declared in the same package but
inside a different language.

Because of this, it is quite easy to create a new language. Simply create a new package named
`lang.a`, and Storm will load `.a`-files using your newly created `Reader`.

Loading
--------

Code is loaded in the following steps:

1. load syntax rules
2. load syntax options
3. load types
4. finalize types
5. load functions

Step 4 is used to set up inheritance for types and other tasks that requires information about other
types present in the same package.

Each `Reader` is handed a set of all files of the same type that are found, since some files may
depend on each other. However, in the case that no files overlap, Storm provides an implementation
of `Reader` that is based on individual files instead of a set of files. To use it, simply inherit
your `Reader` from `core.lang.FilesReader`, and implement the `createFile` member to create a
`core.lang.FileReader` instance for each file. The `FilesReader` will then take care of forwarding
calls for the loading progress to all `FileReader`s.