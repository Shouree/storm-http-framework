Developing in Storm
===================

This section of the manual covers topics related to program development in Storm. It starts simple
with a few tips on how to interpret error messages and the like, and then goes on to describe how to
successively integrate different parts of Storm into an editor (currently, Emacs) to make program
development easier. Finally, it covers how to compile Storm from source code, which allows making
changes to Storm itself, and to develop libraries that interact with native system libraries.

This part of the manual contains the following subsections:

- [**Source References**](md:Source_References)

  Covers how to interpret the source references in error messages produced by Storm.

- [**Compilation Model**](md:Compilation_Model)

  Illustrates some potential surprises about how Storm's compilation model impact when errors are
  reported. In particular, even though languages in Storm are generally statically typed and
  compiled ahead of time, errors may be reported later than you may expect in some situations.

- [**Compile from Source**](md:Compile_from_Source)

  Describes how to compile Storm from source code.

- [**Emacs Integration**](md:Emacs_Integration)

  Describes how to develop in Storm conveniently from Emacs with the help of the Storm plugin. This
  includes how to set up the language server for syntax highlighting and browsing the built-in
  documentation.
