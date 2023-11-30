The Storm Theme
===============

This theme is used to generate the Storm documentation. As such it provides some additional features
to the Markdown syntax to aid in creation of documentation.

First and foremost, the theme needs to know the current version of the Storm system and the date
which it was built on to include them in the documentation. These are both specified as additional
options on the command line (or in the `Options` object):

- `--version=<version>` - the version of Storm the documentation is compiled from. If not specified,
  the version of the current Storm compiler is used.
- `--date=<date>` - the date which the current version was released on.
- `--notes=<path>` - path to the `release_notes.txt` file. If not specified, attempts to use the file `release_notes.txt` in the root of the documentation tree.

Navigation
----------

The Storm theme uses the title of nodes in the documentation tree to generate a table of contents
which the user can use to navigate the documentation. It also generates three links at the bottom of
each page which takes the user to the next or previous page, or up one level.


Extensions
----------

The Storm theme provides the following extensions to the Markdown syntax:

### Syntax Highlighting

The Storm theme uses the standard facilities for highlighting source code using the Storm syntax
highlighting. As such, specifying any language that Storm recognizes enables syntax highlighting for
the corresponding code block. Additionally, a few custom highlighting custom parts of Basic Storm:

- `bsstmt`

  Syntax highlighting for statements in Basic Storm, without the need for declaring a function.

  Additional packages can be included by adding `:use=<package>` as many times as desired. This is
  only necessary to enable additional syntax, since the code is never compiled.

  Furthermore, the special form `:placeholders` can be added to include syntax that allows
  placeholders with the form `<text>` to be used wherever names may normally appear.

- `bsfragment:<rule>`

  Syntax highlighting of a particular rule in the Basic Storm grammar. The `bsstmt` above is roughly
  equivalent to `bsfragment:SExpr`. As with `bsstmt`, additional packages can be included with
  `:use=<package>`, and the `:placeholders` option can be used.

For code elements, the contained code may contain a single line with the syntax: `<?include:...?>`
where `...` is the path of a file relative to the `root` directory. This copies the text from the
file into the code block before it is processed further. This is used to include examples from the
source code of Storm.



### Custom Elements

The following custom elements are available through the `[]` syntax in Markdown. In the Storm theme,
all of these start with a keyword that determines the type of custom element. If the parameter
requires parameters, they are written after a colon (`:`). The following elements are available:

- `downloadbutton:<link>`

  Create the download button on the start page. The parameter `<link>` is the URL that should be
  linked from the button.

- `storm:version`

  Expands to the current Storm version.

- `storm:date`

  Expands to the date of the current Storm build.

- `stormname:<name>`

  Resolve `name` and pretty-print the named entity it refers to. If the name starts with `full:`
  then the full package name is used for functions.

- `storm:releasenotes` (as a code listing)

  Inserts the contents of the release notes file.

### Storm Documentation

A code block with the language `stormdoc` can be used to extract documentation from the built-in
Storm documentation and insert it in the generated documentation. In this case, the contents of the
code block is interpreted as a mini-language that specifies what to include.

In the mini-language, each line is interpreted in turn. Each line contains a name or a part of the
name to something in the name tree. The name may be either absolute or relative. The system
remembers the last absolute name that was resolved and uses it as the starting point for any
relative names that follows. This makes it possible to specify a class- or package name once, and
then only specify the names of the relevant members.

For each name that was successfully resolved, the system inserts the built-in documentation for the
entity in the documentation (after parsing it as Markdown). This behavior can be modified by using
one of the following prefixes:

- `-`

  Add this element to an unordered list. Elements added to a list will also have a title that
  consists of a pretty-printed declaration of the named entity. For member functions, this
  declaration does not include the implicit `this` parameter.

  This prefix must appear first, but can be combined with other prefixes.

  If the line generates multiple entities, each of them are added to their own list element.

- `@`

  Do not add the current element. Only use the name for side-effects (i.e. update the current
  position in the name tree). This allows specifying the remaining elements to be relative to the
  class to reduce typing. For example:

  ```
  @mypackage.MyClass
  - .function1()
  - .function2()
  ```

- `!`

  Remove the first element in the markdown output. This is useful when the first paragraph or
  heading needs to be modified to make the text in the large documentation flow better.

- `.`

  If names starts with a `.`, then they are interpreted as being relative to the last absolute
  name.


For convenience the name resolution in the Storm documentation allows omitting the implicit `this`
parameter of member functions. That is, to refer to the function `f` inside the type `T` it is
enough to write `T.f()` (assuming no other parameters).

Furthermore, the syntax provides a shorthand for including multiple similar entities:

- If the name ends with `.<name>(*)` then all entities with the name `<name>` are added, regardless
  of their parameter list. As such, this is a convenient way to include all overloads of some
  function. The found functions are sorted by appearence in the source file, in order to make the
  ordering consistent and logical according to the original source code.

- If the name ends with `.*` then all child entities of the entity that the string before the `.`
  referred to are included. Again, they are sorted by the order in which they appeared in the source
  file to keep the order consistent and logical.

