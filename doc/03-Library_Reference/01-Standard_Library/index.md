Standard Library
================

This section of the manual describes Storm's standard library. The standard library is located in
the package `core` and a few sub-packages, and contains central data structures and utilities for
languages to use. The types in the standard library also serve as a common language for exchanging
data between languages, since most languages are likely to implment the corresponding concepts in
the language in terms of the Storm standard library.

Most of these types are likely visible by default in languages in Storm. For example, both [Basic
Storm](md:/Language_Reference/Basic_Storm) and [the Syntax
Language](md:/Language_Reference/The_Syntax_Language) searches the `core` package by default.

As mentioned [before](md:..), this part of the manual aims to provide an overview of the central
concepts. Due to the importance of the standard library, this overview is fairly complete and
contains many excerpts from the built-in documentation.
