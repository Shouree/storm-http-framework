Storm
========

?Button(md://Introduction/Downloads)Download Storm?

Storm is a **programming language platform** with a strong focus on **extensibility**. Storm itself is
mostly a framework for creating languages rather than a complete compiler. The framework is designed
to make easy to implement languages that can be extended with new syntax and semantics. Of course,
Storm comes bundled with a few languages (mainly [Basic Storm](md://Basic_Storm)), but these are
separate from the core and could be implemented as libraries in the future. Since these languages
are implemented in Storm, they allow users to create their own **syntax extensions** as separate
libraries. Furthermore, Storm allows languages to **interact** with each other freely and mostly
seamlessly.

Aside from extensibility, Storm is implemented as an **interactive compiler**. This means that Storm is
designed to be executed in the background while programs are being developed. As the compiler is
running in the background, it is able to provide information about the program being developed to
help the developer, much like an IDE. Currently, it is possible to run Storm as a
[language server](md://Storm/Language_server) that provides syntax highlighting for all supported
languages and language extensions to an editor, such as [Emacs](md://Storm/Language_server/Emacs_plugin).
In the future, the language server should be able to provide more semantic information as well.
More information on the language server can be found
[here](http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847).

The following example illustrates some of the possibilities of Storm:

```
?Include:root/presentation/test/simple.bs?
```

In this example, we use a language extension that allows creating presentation slides in a
declarative manner. This extension is not a part of Basic Storm, it is implemented as a language
extension that is included with the `use presentation;` statement on the first line. The language
extension adds a `presentation`-block declares functions that create presentation. This presentation
is then used inside the `main` function to show the presentation. The example also shows that the
language extension is able to execute arbitrary Storm code in most locations by creating a random
caption and storing it in the variable `caption`, which is later used in the slide declarations.

The syntax used to define the syntax language, together with other examples illustrating the
capabilities of Storm can be found on the [Examples](md://Introduction/Examples) page.


Getting started
----------------

If you are interested in Storm and want to learn more, check out some of these sections:

* [Introduction](md://Introduction/) contains instructions describing how to download and install
  Storm, and a couple of examples to show what Storm can do.

* [Storm](md://Storm/) contains documentation on Storm itself. This information is not tied to any
  particular language, but applies to all languages in Storm.

* [Basic Storm](md://Basic_Storm/) contains information about the language Basic Storm that is
  bundled with Storm by default. Refer to this part of the documentation for concrete information
  about syntax and functionality you will see while using Storm.

* [BNF Syntax](md://BNF_Syntax/) contains information on the language used to define syntax in
  Storm. Refer to this part of the documentation if you are interested in creating new languages or
  syntax extensions to other languages.

* [Libraries](md://Libraries/) describes some of the libraries included with Storm. For example, a
  library for creating [GUI applications](md://Libraries/Gui), for [sound
  playback](md://Libraries/Sound), and for making [presentations](md://Libraries/Presentation).

* [Programs](md://Programs/) contains documentation on a number of programs written in Storm and
  included in the download. These show what is possible to do in Storm.

Note that the main goal of the documentation provided here is to get you started in using the
language and to give an understanding of the language. It will not discuss specific APIs or the
standard library in depth. For that kind of documentation, please refer to the built-in
documentation in Storm. In the Basic Storm REPL, type `help <name>` to access documentation for
entities in the system. For example: `help core:Str` will tell you about the string type and its
members. `help core:Str:find` will tell you that there are two overloads of `find`.

You can also access the documentation in Storm using the
[Emacs plugin](md://Storm/Language_server/Emacs_plugin). Run the command `M-x storm-doc`
and enter the name of the thing you want to see documentation for. The Emacs plugin allows interactive
browsing of the documentation and provides auto completion for the name entry which makes it easy
to explore the contents of packages.


Contact
--------

If you have any questions or requests regarding Storm, please contact me at
[info@storm-lang.org](mailto:info@storm-lang.org).
