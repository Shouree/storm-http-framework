Documentation in Markdown
=========================

The Markdown library also provides utilities for generating documentation from a set of Markdown
files. This part of the library resides in the [stormname:markdown.doc] package.

At a high level, the documentation system searches a directory for all Markdown files (with the
extension `.md`). It then copies the structure of the hierarchy to another directory, but generates
HTML files that correspond to the contents in the original Markdown files instead. The system also
verifies any links between the individual Markdown pages, so that they exist. They are also
rewritten so that they link to HTML documents rather than Markdown documents. To determine how the
HTML files look, a *theme* is used.

The library has two entry points: one that is designed to be used from other programs in Storm, and
one that is designed to be used on the command-line. The command line interface can be called as
follows:

```
./Storm -f markdown.doc.generate -- <title> <theme> <input> <output> [options]
```

The placeholders above have the following meaning:

- `<title>`

  The title of the documentation as a whole.

- `<theme>`

  The name of the theme to use.

- `<input>`

  Path to the root of the Markdown files.

- `<output>`

  Path to the location to place the finished output. The directory is created if it does not already exist.

- `[options]`

  Zero or more options of the form `--<name>` or `--<name>=<value>` that modifies the behavior of
  the system. By default the following options are available, but any remaining options are passed
  to the theme for further processing. As such, themes may introduce additional options.

  - `--quiet` or `-q`

    Quiet. Suppress progress output.

  - `--clear`

    Clear the output directory before generating any output. Note: This removes all files and
    directory in whatever directory is used as the output. This can have unintended consequences if
    not used with care.

  - `--numbers`

    Keep the numbers in the names of the Markdown files in the HTML output. Otherwise they are removed.


There is also an alternate entry point that makes it convenient to generate the Storm documentation
from the command line:

```
./Storm -f markdown.doc.generateStormDoc -- <output> [options]
```

For example, it is possible to generate the Storm documentation in the `html` directory as follows:

```
./Storm -f markdown.doc.generateStormDoc -- html
```

The programmatic interface is similar, and consists of the following two overloads to the function `generate`:

```stormdoc
- markdown.doc.generate(markdown.doc.Options)
- markdown.doc.generate(Str, Str, core.io.Url, core.io.Url)
```

From the command-line, the library can be called as follows:



Directory Structure
-------------------

The documentation system expects the documentation to be organized as a tree. Each node in the tree
has a title and contains a Markdown document that will eventually be rendered as HTML. Note that the
title of a node is only used to refer to the node, and possibly also to generate a table of
contents. The title is not displayed in the generated document automatically.

Due to the tree structure, it is possible to uniquely identify a document through a path of titles.
For example, `Topic_A/Section_C`. This idea makes it fairly straight-forward to see how the Markdown
documents can be stored in the file system.

Since it is not possible to store data in the directory itself in a file system, data for `Topic_A`
in the example above needs to be stored elsewhere. To do this, a similar scheme that is used on
web-servers is used. Namely, if the node `Topic_A` is stored as a directory, then the corresponding
Markdown document is stored in the file `index.md` inside the directory `Topic_A`.

Secondly, it is necessary for the author to specify the order in which the different documents shall
appear. Otherwise, `Topic_B` might appear before `Topic_A`. This is solved by prefixing the title of
all nodes with an integer of some fixed length. For example, the node `Topic_A` would be named
`01-Topic_A`, and the node `Topic_B` would be named `02-Topic_B`. The names can then be sorted
lexiographically in order to find the intended order.


These two observations lead us to the naming scheme used by the documentation system: the root of
the documentation tree is located at some directory in the file system (specified on the
command-line). Each file and directory below the root directory, except for files named `index.md`,
correspond to a node in the directory tree. Child nodes are ordered by sorting the names of the
files lexiographically. The title of each node is derived from the name of the corresponding file or
directory. The title includes everything after the first dash (`-`), and with underscores (`_`)
replaced with spaces. This means that the structure of file names proposed above (`01-Title_B`) is
overly restrictive. Any sequence of characters before the dash (`-`) can be used to order the nodes.
The system only uses them as a way to conveniently order nodes during a lexiographical sort. After
that, they are removed.

As an example, a documentation tree may be stored like below in the file system. Directories are
denoted by a trailing slash (`/`):

```
root/
|
|- index.md
|
|- 01-Topic_A/
|  |- index.md
|  |- 01-Section_A.md
|  |- 02-Section_B.md
|
|- 02-Topic_B/
|  |- index.md
|  |- 01-Section_A.md
|  |  |- index.md
|  |  |- 01-Subsection_A.md
|  |- 02-Section_B.md
|
|- 03-Topic_C.md
```

One detail has been left out so far. Namely, the naming scheme does not specify a title for the root
node in the tree. The name for the root node is therefore specified as a parameter when invoking the
system instead.


Output
------

By default, the system creates a directory structure that resembles the structure of the input.
There are two differences. The first is that all `.md` files have been replaced with `.html` files.
The second is that files are named according to the title of the node (i.e. everything before the
first `-` has been removed). This makes it possible to reorder nodes in the documentation without
breaking existing links. As such, the output for the example above would look like this:

```
output/
|
|- index.html
|
|- Topic_A/
|  |- index.html
|  |- Section_A.html
|  |- Section_B.html
|
|- Topic_B/
|  |- index.md
|  |- Section_A/
|  |  |- index.html
|  |  |- Subsection_A.html
|  |- Section_B.html
|
|- Topic_C.html
```

If desired, it is possible to keep the numbers by passing `--numbers` on the command line, or
setting `stripNumbers` to `false` in the `Options` object.


Links
-----

The documentation system is able to automatically verify links between pages in the documentation.
To differentiate links that refer to other Markdown documents from other links (e.g. to external
resources), the system uses the special protocol `md:` for links to other Markdown documents. All
links that start with `md:` are inspected by the system. The part after `md:` is interpreted as a
path and is replaced with a corresponding relative link in the HTML output. If the target of the
link is not found, the system prints a warning.

Both absolute and relative paths are supported. Absolute paths start with `md:/` while relative
paths do not have a leading slash after the `md:`. The remainder of the path is a list of slash
(`/`) delimited node titles. Note that the title of the node is used rather than the filename. This
means that both the number in the filename and any file extensions are omitted. Again, this is to
allow nodes to be reordered without breaking links.

For example, to create a link from `02-Topic_B/02-Section_B.md` to `02-Topic_B/01-Section_A/`, one
could write the path as either `md:Section_A` (a relative path), or as `md:/Topic_B/Section_A`.
Again, note that both the numbers and file extensions are omitted in the links.


Markdown Extensions
-------------------

A few extensions to the Markdown syntax are available when using the documentation system:

- Inline HTML can be specified in a code block with the special language `inlinehtml`. This allows
  implementing functionality that is outside the capabilities of the Markdown library.


Themes
------

A *theme* is used to embed the HTML that corresponds to the converted Markdown documents into a
skeleton to create a full HTML page. It also has the ability to perform additional modifications to
the Markdown document before it is converted into HTML in order to add additional functionality.

A theme is simply a function in the package [stormname:markdown.doc.themes] that returns an instance
of (a subclass of) the [stormname:markdown.doc.Theme] class. The `Theme` class has the following
members that a theme may override:

```stormdoc
@markdown.doc.Theme
- .options(*)
- .toHtml(*)
- .copyFiles(*)
```

Currently, only one theme is available: the theme used to generate the Storm documentation. This
theme has a number of features that makes it more convenient to document Storm. It is documented
[here](md:The_Storm_Theme).


The file `markdown/doc/themes/util.bs` contains a few utilities for use when creating a new theme.
Below are some examples:

```stormdoc
@markdown.doc.themes
- .copyFiles(*)
- .copyAllFiles(*)
- .copyFile(*)
- .replaceMarkers(*)
```

Furthermore, the package `markdown.doc.highlight` contains the function `highlightSource` that
implements syntax highlighting for languages that are recognized by Storm:

```stormdoc
@markdown.doc.highlight
- .highlightSource(core.io.Url, Str, Str)
- .highlightSource(core.io.Url, markdown.CodeBlock)
```
