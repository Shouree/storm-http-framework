Basic Storm
===========

Basic Storm is one of two languages that are built into Storm by default. One of the design goals of
Basic Storm is that it should match the features offered by the language-agnostic parts of Storm
closely (these are described [here](md:../Storm)). Therefore, Basic Storm is well suited to
illustrate how Storm works in general, and to explore Storm itself.

Basic Storm itself is an imperative language, with syntax that resembles C++ and Java. As such,
developers that are familiar with these languages are likely to be able to pick up Basic Storm
quickly.

Apart from closely matching the capabilities of Storm itself, Basic Storm have two additional
qualities that illustrate the possibilities of the platform. First, Basic Storm is extensible. This
means that it is possible to create syntax extensions for Basic Storm as libraries. The section on
[libraries](md:/Libraries) contain a few examples of such libraries. Secondly, it can be used as a
compilation target for other languages. Since all components of the language Basic Storm are exposed
in the name tree, they are accessible from other languages in the system. Therefore, there are
multiple options when developing a new language in the system. The new language may either chose to
generate code in the intermediate representation directly. This is what Basic Storm does. It might
also chose to emit an abstract syntax tree that can be understood by Basic Storm, and utilize the
code generation abilities of Basic Storm to do the actual code generation.

Finally, it is worth mentioning that Basic Storm is not special to Storm itself in any way except
that it is included with Storm by default. This means that it is possible to implement an equaly
capable language in any language in Storm itself.

The remainder of this section is dedicated to introducing the language Basic Storm and its features.
The section will end by providing an overview of some of the most important parts of the language
implementation. This is useful when creating language extensions to Basic Storm, or when using Basic
Storm as a target for code generation.
