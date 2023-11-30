Storm
========

[downloadbutton:md:/Downloads]

Storm is a **programming language platform** with a strong focus on **extensibility**. Storm itself
is mostly a framework for creating languages rather than a complete compiler. The framework is
designed to make easy to implement languages that can be extended with new syntax and semantics. Of
course, Storm comes bundled with a few languages (most importantly [Basic
Storm](md:/Language_Reference/Basic_Storm)), but these are separate from the core and could be
implemented as libraries in the future. Since these languages are implemented in Storm, they allow
users to create their own **syntax extensions** as separate libraries. Furthermore, Storm allows
languages to **interact** with each other freely and mostly seamlessly.

Aside from extensibility, Storm is implemented as an **interactive compiler**. This means that Storm
is designed to be executed in the background while programs are being developed. As the compiler is
running in the background, it is able to provide information about the program being developed to
help the developer, much like an IDE. Currently, it is possible to run Storm as a [language
server](md:/Getting_Started/Developing_in_Storm/Emacs_Integration) that provides syntax highlighting
for all supported languages and language extensions to an editor, such as Emacs. In the future, the
language server should be able to provide more semantic information as well. More information on the
language server can be found [here](http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847).

The example below illustrates the capabilities of Storm:

```bs
<?include:sql/tests/example.bs?>
```

The example uses the SQL library, which provides a *syntax extension* to Basic Storm that allows
treating SQL queries as expressions. Together with the database declaration at the top of the
example, this allows the SQL library to type-check queries. The type information is also used to
make it possible to extract the results in a convenient and type-safe way. Furthermore, since the
extension knows which parameters originate from variables in the code, it is able to automatically
use prepared statements to avoid problems with SQL injection.


Getting Started
----------------

If you are interested in Storm and want to learn more, the [getting started](md:/Getting_Started)
page is a good place to start. Apart from the getting started section, this page also contains the
manual for Storm among other things.

- [**Getting Started**](md:/Getting_Started)

  This part of the manual aims to introduce Storm by providing step-by-step instructions that
  illustrate how to use the system. These instructions start simple by providing detailed
  instructions on how to install Storm and run programs in Storm. From there, it gradually builds up
  to more advanced examples that illustrate the unique features of Storm. These examples are not
  ment to be complete, but rather to provide a starting point for exploring the system further
  through the [reference](md:/Language_Reference) [sections](md:/Library_Reference) of the manual
  and the [built-in documentation](md:/Getting_Started/Running_Storm/Getting_Help).

- [**Language Reference**](md:/Language_Reference)

  The language reference provides a detailed description of the different languages used in Storm.
  It starts by introducing the ideas behind Storm itself in a language-agnostic setting, and then
  continues to describe the general purpose language [Basic
  Storm](md:/Language_Reference/Basic_Storm), [the Syntax
  Language](md:/Language_Reference/The_Syntax_Language), and the [intermediate
  representation](md:/Language_Reference/Intermediate_Language) in more detail.

- [**Library Reference**](md:/Library_Reference)

  To complement the language reference, the library reference covers the library functions that are
  available. This covers both the [standard library](md:/Library_Reference/Standard_Library)
  provided by Storm, the interfaces provided to aid in [creating
  languages](md:/Library_Reference/Compiler_Library), and other libraries that are included with the
  system.

- [**Programs**](md:/Programs)

  A number of larger programs have been developed in Storm, and are included with Storm by default.
  They are documented in this part of the manual.

- [**Downloads**](md:/Downloads)

  This page lists the binary releases of Storm in a way that is easy to access.


Contact
--------

If you have any questions or requests regarding Storm, please contact me at
[info@storm-lang.org](mailto:info@storm-lang.org).
