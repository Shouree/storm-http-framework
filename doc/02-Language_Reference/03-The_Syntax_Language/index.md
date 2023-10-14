The Syntax Lanugage
===================

The Syntax Language is one of the two languages that are built into Storm by default. In contrast to
Basic Storm, the Syntax Language is not a general purpose language. Rather, it is a domain specific
language for declaring BNF grammar and transforms that specifies how to interpret the grammar.

As with Basic Storm, the Syntax Language is designed to be extensible. For example, it is possible
to create syntax extensions to the Syntax Language in itself and Basic Storm. Furthermore, the
Syntax Language is designed to make it easy to create other extensible languages since the Syntax
Language provides *both* a means of extending the syntax of a language, *and* a way to specify the
semantics of the extension at the same time. With the help of Storm, The Syntax Language achieves
this in a typesafe manner, all the way from the source text to the abstract syntax tree of the
target language.


The remainder of this section describes the Syntax Language in full. Since the Syntax Language deals
with many similar terms, readers are encouraged to start by looking at the
[terminology](md:Terminology) section of the manual.


Finally, a note for the curious reader: all code generation in the Syntax Language is implemented
using Basic Storm: The Syntax Language creates abstract syntax trees in Basic Storm and thus
utilizes the logic in Basic Storm to handle the low-level code generation. This is possible even
though Basic Storm depends on the Syntax Language to parse source code.
