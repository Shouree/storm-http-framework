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

The documentation system organizes documentation in a tree-like way. Each node in the tree consists
of a document that is rendered as HTML. Furthermore, the children of each node are ordered, so that
it is possible to organize the documentation in a logical order.

In the file system, this structure is stored as follows. Each node in the documentation tree is
given a name of the following form: `n-a_b_c`, where `n` is a number (with any number of digits)
that determines the order in which the node shall appear. The remaining part (`a_b_c`) determines
the title of the node in the documentation. The title of the node is the characters after the first
`-`, but with underscores replaced by spaces. This naming scheme allows ordering the nodes simply by
sorting their names lexiographically.

Leaf nodes in the tree may then be stored as files (with a `.md` extension) in the file system.
Non-leaf nodes are stored as directories with the name of the node. The actual contents of the node
is stored in a file named `index.md` inside the directory.

To illustrate the idea, consider the example below. Directores are denoted with a trailing slash.

```
root/
|
|- index.md
|
|- 01-About/
|  |- index.md
|  |- 01-Authors.md
|
|- 02-Reference/
|  |- index.md
|  |- 01-Functions.md
|  |- 02-Types_and_Enums.md
|
|- 03-Download.md
```

As can be seen above, the directory structure used for markdown documents closely mirror the
conventions when working with HTML documents on a webserver: all directories contain a file named
`index.md` that is used when referring to the directory itself, rather than a file inside the
directory. Furthermore, only nodes that contain children need to be stored as directories. It is,
however, possible to store nodes without children as a directory that only contains an `index.md`
file if desired.
