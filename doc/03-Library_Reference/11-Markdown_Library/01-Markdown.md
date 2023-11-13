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

Due to Markdown relying on indentation to delimit structures like lists, it is a good idea to
consistently use spaces for indentation. If tabs are encountered, the Markdown parser assumes that
the width of a tab is equal to four spaces at all times (i.e. it works for cases where tabs appear
at the start of a line, not if there are other spaces before the tab).


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

Two types of lists are supported: ordered lists and unordered lists. Unordered lists are written
using `-`, `+` or `*` followed by at least one space to mark each element. Ordered lists are instead
marked using a number followed by a dot and a space. The content of each element then has to be
indented according to the first line. A list may contain multiple paragraphs, or even nested lists.

An unordered list may look as follows:

```
- first element

  second paragraph of first element

- second element

- third element
- fourth element
```

As can be seen above, it does not matter if empty lines are present between elements. Rather, the
auto-fill functionality of some editors work better when there is a blank line between elements. Of
course, blank lines are required to separate paragraphs, and some elements.

Similarly an ordered list may look as follows:

```
1. first element

   second paragraph of first element

2. second element
3. third element
```

Note, however, that the numbering of elements is not verified nor respected. That is, it is possible
to number all elements 0 and still get a properly numbered list in HTML, since the syntax simply
emits `<li>` tags and lets the HTML renderer manage numbering.



Text Formatting
---------------

Text that appears almost anywhere can be formatted using any of the following syntaxes. It is
generally possible to escape symbols that have other meanings using backspace (`\`), except for in
the inline code syntax.

- **Bold** text:

  ```
  **text**
  ```

- *Italic* text:

  ```
  *text*
  ```

  Note: **bold** and *italic* can be combined into ***bold italics***.

- Inline `code`:

  ```
  `text`
  ```

- Links:

  ```
  [text](target)
  ```

  Here, `target` is a URL that is inserted in the finished HTML page. The text can be formatted
  using bold, italics, etc.


Code Blocks
------------

```inlinehtml
<p>
  For larger pieces of code, three backticks (<code>`</code>) can be used to indicate the start- and
  end of a larger code block. It is also possible to indicate the language of the code right after the
  backticks. The Markdown library itself does not use the language, but it makes it available to
  users of the document representation for things like syntax highlighting.
</p>
```


In a context where the code block is indented, the parser will remove any whitespace consumed by the
indentation from the content of the code block. This means that the code inside the block should be
indented to at least the same level as the starting and ending backticks.

The code block is best treated as a paragraph. That is, it is best to leave an empty line both
before and after it.

For example:

```inlinehtml
<pre>paragraph of regular text

<!-- -->```bs
void main() {
    print("Hello!");
}
<!-- -->```

another paragraph</pre>
```

Of course, the text `bs` can be replaced with any string, or entirely omitted.


Extension Points
----------------

The markdown format provides a few points where the Markdown language can be extended without
modifying the parser itself. They are as follows:

- The link syntax can be used without an URL (e.g. `[text]`). This has no meaning in regular
  Markdown, but the dialect in this library parses it into a separate element. This element is
  available in the document representation for libraries to inspect and modify.

- In the proper link syntax, the URL can also be used to add custom annotations, for example by
  adding a custom "protocol". Then it is quite easy to find links with certain targets and replace
  them with something else.

- The language name in code blocks is not interpreted by the parser. This allows libraries to
  replace it with something else. For example, the documentation library uses the special string
  `inlinehtml` to allow inserting custom HTML code for things that are not supported by the library.
