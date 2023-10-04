Packages and Files
===================

As previously mentioned, Storm itself is language agnostic. As such, rather than supporting a
pre-determined set of file types, Storm provides a generic mechanism for dispatching compilation of
different file types to different language implementations. This section covers how Storm determines
what languages to use for different file types, and how this is implemented in the `Package` class.
This information can therefore be used to implement new languages. This information is, however,
intended as a reference. Examples can be found in the [getting started](md:/Getting_Started/Tutorials) section.


File Types and Languages
------------------------

As mentioned [previously](md:The_Name_Tree), the `Package` entity in the name tree corresponds to a
directory in the file system. The `Package` is then responsible for determining when and how to load
its contents from the file system. For sub-directories, the `Package` entity simply creates entities
that correspond to each of the sub-directories. Whenever a part of the system requests a name that
does not refer to a sub-package, the `Package` entity needs to load entities from all files in the
directory. This is done as follows:

1. Collect a list of files, grouped according to the file extension.

   The `Package` entity first collects a list of all files in the directory, and groups them by
   their file extension. For example, the files `a.bs` and `b.bs` will belong to the same group
   since they have the same extension. If a file `syntax.bnf` also exists, it will be in its own
   group since the extension differs.

2. Create a `lang.PkgReader` for each group.

   The `Package` then creates an instance of the `core.lang.PkgReader` class for each group of
   files. The `PkgReader` class is an abstract class that each language is expected to subclass in
   order to implement language-specific loading. Each instance of the `PkgReader` class thus
   corresponds to a single language that loads all of its source files.

   To create a `PkgReader`, the system attempts to find the function
   `lang.<ext>.reader(core.Array<core.io.Url>, core.lang.Package)`. In this case `<ext>` is the file
   extension of the group. For example, the `PkgReader` for Basic Storm is created by calling the
   function `lang.bs.reader(...)`, since Basic Storm uses the file extension `bs`. The first
   parameter to the function is an array of all files with this extension in the package. The second
   parameter is a reference to the `Package` that attempts to load code. This is where the language
   is expected to place any loaded entities.

3. Coordinate loading.

   To handle dependencies between languages, the `Package` entity is also responsible for
   coordinating the loading process. The `PkgReader` class contains the following member functions.
   Each corresponds to a step in the loading process:

   1. `readSyntaxRules`

      The first step is to read any syntax rules (i.e. non-terminals), create entities for them, and
      place them in the package. In this way, it is possible to look up rules referred from
      productions in the next step.

   2. `readSyntaxProductions`

      The next step is to read any productions, create entities for them, and place them in the
      package. During this process it is possible to resolve the rule that the production is a part
      of, as well as any rules used in the production itself. Any semantic actions may, however, not
      be resolved immediately. This needs to be delayed until when the transform function is called.

   3. `readTypes`

      This step involves reading types, creating entities for them, and placing them in the package.
      At this point, only an empty type should be added. In particular, any super-class or named
      thread should not be resolved yet. Furthermore, the contents of the type is expected to be
      lazy-loaded as needed.

   4. `resolveTypes`

      During this step, the reader is expected to resolve any super-classes or named threads that
      were specified alongside the types. This ensures that regardless of the order in which files
      are parsed, types specified in other files (and languages) will be visible at this step.

   5. `readFunctions`

      After loading types, remaining entities such as functions are loaded. At this point, types are
      loaded which means that it is possible to resolve parameter types.

   6. `resolveFunctions`

      Finally, if any additional work needs to be done to finalize other definitions, it can be done
      here. For example, Basic Storm uses this step for finalizing global variable declarations.


   The `PkgReader` class executes the above-mentioned steps in the order mentioned above. When
   multiple `PkgReader`s exist, it ensures that the first step for all readers have been executed
   before the second is executed. This ensures that dependencies between languages are handled
   properly. For example, that Basic Storm is able to find types defined in the Syntax Language.

   It is worth mentioning that a `PkgReader` does not have to implement all steps above. For
   example, Basic Storm does not support syntax definitions and therefore does not do anything in
   the first two steps. Furthermore, the system does not verify that languages follow the intent
   behind these steps. As such, languages may choose to use them as they see fit to ensure that
   dependencies are handled according to the semantics of the particular language.


Files with Special Meaning
---------------------------

The `Package` entity recognizes a few additional files without extension that have special meaning.
These are as follows:

- `README`

  If the file `README` exists, the `Package` will load its content and display as documentation for
  itself when requested. As such, creating a `README` file is a good way to provide an overview of
  the contents of a package, or a library. In the Basic Storm top loop, the documentation can be
  viewed by typing `help <package>`.

- `export`

  When present, this file contains a list of package names that are to be *exported*. All exported
  packages are automatically included whenever the current package is imported elsewhere. For
  example, if the package `a` exports package `b`. Then, when a language imports package `a`, then
  the system expects package `b` to be automatically included as well. This feature is particularly
  useful when developing a domain-specific language that embeds syntax of another language. One
  example of this is the `ui` library. The `ui` library provides syntax for creating windows in
  Basic Storm. This syntax utilizes the generic layout syntax in `layout`. Because of this, the UI
  library exports the `layout` package. Otherwise, if one were to only import the `ui` library, the
  custom syntax would not behave as expected since the syntax from `layout` would not be visible.


Utilities
---------

The `lang.PkgReader` interface is designed to allow for maximum flexibility for languages. As such,
it makes no assumptions about the content of the files, nor if any files have special meaning. Due
to the flexibility, Storm is able to use the standard `PkgReader` interface to load dynamic
libraries (i.e. `.so` or `.dll`), which are binary files.

Since many languages consist of text files that can be treated individually, Storm also provides a
`FilePkgReader` to make this use case more convenient. The `lang.FilePkgReader` class inherits from
`lang.PkgReader`, so a `FilePkgReader` can be used anywhere Storm expects a plain `PkgReader`.

A `FilePkgReader` treats each file individually, and assumes that all files consist of text that
will eventually be parsed by some grammar using the system parser. As such, the `FilePkgReader`
takes care of reading the contents of the files, and creates one `FileReader` that corresponds to
each file. To support languages where the syntax may change midway through a file (e.g. importing
new syntax in Basic Storm), each `FileReader` may choose to treat the file as a sequence of
different *parts*. Each part may have their own syntax, and each part may affect the grammar
available any parts that follows the current one.

While Storm does not require that the `FilePkgReader` is used, languages that wish to provide syntax
highlighting in [the language server](md:Language_Server) need to use the `FilePkgReader` interface.
