Parsing
=======

To aid in creating extensible languages, the compiler library also contains a parser that is made to
be easily extensible, alongside with the [syntax
language](md:/Language_Reference/The_Syntax_Language) for defining the grammar used by the parser.
As with previous sections of the compiler library, this part of the documentation focuses on the
interfaces themselves. The conceptual parts are described in the corresponding section of the
[language reference](md:/Language_Reference/The_Syntax_Language).

**Note:** The parser described here is powerful and is able to parse all context-free languages.
This does, however, come at a cost in complexity and performance. For application code that parses
simple input formats, using the [parser library](md:/Library_Reference/Compiler_Library/Parsing),
which provides simpler but more performant parsers for such tasks.


Source Positions
----------------

An important part of creating a language is to be able to refer back to the source code to report
problems. To achieve this, it is necessary to have a representation of a location in the source
code. The value [stormname:core.lang.SrcPos] has this role. It stores a source position in the form
of a [stormname:core.io.Url] that refers to a file, and two integers that refer to a range of
characters in the file. For the purposes of the integers, a text file is thought to consist of an
array of codepoints, and line-endings are represented as a single codepoint (`\n`) regardless of how
they are stored in the file. The integers are indices into this array of codepoints.

Instances of `SrcPos` are produced by the parser to indicate source locations. They are also
typically found in parse trees and abstract syntax trees to properly report errors.

The `SrcPos` class has the following members:

```stormdoc
@core.lang.SrcPos
- .__init(*)
- .file
- .start
- .end
- .any()
- .empty()
- .extend(*)
- .firstCh()
- .lastCh()
```


Parsers
-------

The compiler library provides a generic and powerful parser that is able to parse all context-free
grammars. Due to its tight interaction with grammars from the name tree, this parser is always
executed on the `Compiler` thread. Since the parser is powerful, it is comparatively expensive to
set up to parse strings. This is generally fine when parsing source code, but might not be desirable
for interactive applications, or for parsing file formats. The former case is addressed by the
[stormname:core.lang.InfoParser] (described below) and the latter is addressed by the [parser
library](md:/Library_Reference/Parser_Library).


### The Main Parser

The main parser is implemented as the actor `core.lang.Parser<T>`. The parameter `T` is the name of
a rule in the name tree that shall be used as the starting point in the grammar. The information
from this rule is also used to provide a type-safe way of extracting the parse tree, which later
allows executing syntax transforms in a type-safe manner.

The parser initially only considers productions that are in the same package as the starting rule to
be visible. To consider productions, more can be added using the `addSyntax` function. The
`addSyntax` function automatically takes exported packages into account. This mechanism is used by
Basic Storm to provide extensible syntax.

The parser class has the following members:

- [stormname:core.lang.Parser(lang.bs.SIncludes).__init()]

  Creates the parser.

- [stormname:core.lang.ParserBase.addSyntax(core.lang.Package)]

  Make all productions in `pkg` visible to the parser.

- [stormname:core.lang.ParserBase.parse(core.Str, core.io.Url)]

  Parse the string `str` using the currently visible grammar. Returns `true` if a match was
  found, and `false` otherwise. Note that a match always starts from the beginning of the string,
  but may end before the end of the string. To ensure that the entire string was matched, use
  `hasError` or inspect `matchEnd` after parsing. The parser always attempts to match as much as
  possible of the input.

  The parameter `file` is used to create `SrcPos` instances in the parse tree and for reporting
  errors.

- [stormname:core.lang.ParserBase.parse(core.Str, core.io.Url, core.Str.Iter)]

  Like above, but starts parsing at `start` rather than at the beginning of the string.

- [stormname:core.lang.ParserBase.clear()]

  Clears all parse-related information. Included packages are, however, retained.

- [stormname:core.lang.ParserBase.hasError()]

  Check if an error message is available. If `matchEnd` does not refer to the end of the string,
  this is always true (since the error could be "unexpected character X" if nothing else).

- [stormname:core.lang.ParserBase.hasTree()]

  Check if the parse was successful, and that we can produce a parse tree. This is equivalent to the
  return value of `parse`.

- [stormname:core.lang.ParserBase.matchEnd()]

  Get an iterator to the end of the match.

- [stormname:core.lang.ParserBase.error()]

  Get the current syntax error, if `hasError` returned true.

- [stormname:core.lang.ParserBase.throwError()]

  Throw the current syntax error, if any.

- `T tree()`

  Create and return the parse tree from the match. Assumes that `hasTree` returns true. Throws
  otherwise. Note that `T` is the same as the parameter that represents the start of the parse.

- [stormname:core.lang.ParserBase.infoTree()]

  Create and return an [info tree](md:/Language_Reference/The_Syntax_Language/The_Info_Tree) that
  describes the current match.


### The Info Parser

In certain situations it is useful to be able to dynamically specify the starting point of a parse
(e.g. in the language server). This is allowed by the actor [stormname:core.lang.InfoParser]. It
provides a slightly different interface to the same parser as described above. All members are the
same, except that `tree` is missing since it is not possible to achieve in a type-safe manner. It is
thus only possible to extract info trees from the info parser.

Note that since the parser in Storm needs to create parse-tables to parse efficiently, it is
preferred to re-use the same parser instance for as long as possible. This makes the info parser
beneficial in situations like the language server. Since it is possible to modify the start
production, it means that large portions of the parse tables can be re-used, even with different
starting points in the grammar.

The info parser has the following members in addition to the generic parser:

- [stormname:core.lang.InfoParser.__init(lang.bnf.Rule)]

  Create an info parser.

- [stormname:core.lang.InfoParser.root(lang.bnf.Rule)]

  Set the root rule of the parse after creation.

- [stormname:core.lang.InfoParser.root()]

  Get the current root rule of the parse.

- [stormname:core.lang.InfoParser.parseApprox(core.Str, core.io.Url)]

  Parse a string using error recovery. Returns an `InfoErrors` object that describes how "bad" the
  match was. There are also overloads where the starting position is specified, and that provides an
  `InfoInternal` node that is used as a state for the non-context free parts of the grammar.

- [stormname:core.lang.InfoParser.fullInfoTree()]

  Return an info tree that is guaranteed to match to the end of the parsed string, regardless of the
  length of the match.


Parse Trees
-----------

All nodes in a parse tree are derived from the actor [stormname:lang.bnf.Node]. The type defined by
each rule inherits from `Node`. Then types for individual productions inherit from the type defined
from the rules. As such, all nodes in a parse tree have the following members in common:

```stormdoc
@lang.bnf.Node
- .__init(*)
- .pos
- .children()
- .allChildren(*)
```

Note that the `transform` function is defined in a derived class.


Info Trees
----------

As mentioned in the [language reference](md:/Language_Reference/The_Syntax_Language/The_Info_Tree),
an info tree is a representation of the parse tree that includes all matches, even those not
captured in the grammar. It is therefore useful for syntax highlighting and indentation.

Since the info tree contains matches for the entire input string, it aims to be compact to avoid
excessive memory usage. As such, the representation is not designed to be modified after creation
(apart from re-linking the tree). Since the info tree is complete, it is possible to re-construct
the input string from the info tree.

The info tree consists of three actors: [stormname:lang.bnf.InfoNode], which is a generic node,
[stormname:lang.bnf.InfoInternal], which is an internal node, and [stormname:lang.bnf.InfoLeaf],
which is a leaf node.

### Info Nodes

The [stormname:lang.bnf.InfoNode] contains the interface common for both child- and internal nodes.
It has the following members:

```stormdoc
@lang.bnf.InfoNode
- .color
- .length()
- . error()
- .delimiter()
- .leafAt(core.Nat)
- .indentAt(core.Nat)
- .format()
- .parent(*)
```


### Internal Nodes

The [stormname:lang.bnf.InfoInternal] is an internal node in an info tree. It thus corresponds to
matching a production in the text. It contains the following additional members:

```stormdoc
@lang.bnf.InfoInternal
- .indent
- .production()
- .[](core.Nat)
```

### Leaf Nodes

The [stormname:lang.bnf.InfoLeaf] is a leaf node in an info tree. It corresponds to matching a
single regular expression to the input, and contains the actual string that was matched. Since error
recovery might have been used to parse the input, information about the regex may not always be
present, and the contained string may not always match the regex.

```stormdoc
@lang.bnf.InfoLeaf
- .matches()
- .matchesRegex()
```
