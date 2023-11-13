Supported Markdown
==================

This page describes Markdown as understood by the Markdown library in Storm. Note that there are a
number of different dialects of Markdown in the wild, with slight differences. This library
implements a fairly small set of the Markdown language, and due to the use of a formal parser, it is
perhaps a bit stricter in what is accepted compared to other Markdown parsers. Even so, the language
is complete enough to allow writing the Storm documentation (at least, with some minor extensions).

The markdown library uses the [parser library](md:/Library_Reference/Parser_Library) to generate a
backtracking recursive descent parser (even though Markdown is not context-free, the extensions
provided by the parser library allows parsing indentation sensitive languages). As such, the grammar
used to parse Markdown is available in the file `root/markdown/parse.bs` for the curious reader.


Headings
--------

There are two ways to specify headings in the Markdown language. The first is to use number signs
(`#`) before a line of text. The number of signs determine the level of the heading. For example:

```
# First level heading

## Second level heading

### Third level heading

...
```

The library supports arbitrary deeply nested headings. However, the final HTML might not be very
useful if headings are nested too deeply.

There is also an alternative notation for the first two headings. Namely, to underline a single line
with either equal signs (`=`) or dashes (`-`):

```
First level heading
===================

Second level heading
--------------------
```


Paragraphs
----------

Any sequence of non-empty lines are considered to belong to the same paragraph, regardless of
whether it contains line endings or not. As such, to create a new paragraph it is enough to leave at
least one blank line. It is also a good idea to leave a blank line between headings and paragraphs
to avoid ambiguity.

For example:

```
first paragraph
consists of multiple
lines

second paragraph starts
here because the previous
line was blank
```

A line is considered to be blank even if it contains whitespace characters.


Lists
-----

Text Formatting
---------------


Links
-----


Code Blocks
------------


Extension Points
----------------

- `[]`
- code title
