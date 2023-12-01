Extensibility
=============

Basic Storm is an extensible language, and it is possible to develop language extensions for it.
Extensions can generally be encapsulated in a separate package, that the user of the extension can
selectively include with `use` statements.

The mechanisms in Basic Storm allow a number of different type of extensions, of varying complexity:

- Extensions to the interface of other classes:

  Since Basic Storm utilizes uniform function call syntax, it is possible to create free functions
  that seem like they extend the interface of a class from a different package. It is therefore
  possible to create a package that seemingly adds to the public interface of classes in other
  packages by simply creating free functions with the appropriate first parameter.

- New options for declarations:

  New options for types can be created by simply creating a free function that accepts a
  `core:lang:Type` as its first parameter. Options that need to accept parameters, or options for
  functions require extending the syntax of the language. This is done by providing new productions
  in the extensions' package. These are then only visible whenever the extension has been `use`d.

- New syntax in functions:

  New syntax for the code in functions can be provided by extending the syntax of the language. The
  extension's package provides additional productions that are only visible when the package are
  used. These productions then instantiate new or existing nodes of the Basic Storm abstract syntax
  tree, to eventually generate code that implements the semantics of the new syntax.

  The grammar is implemented in the [syntax language](md:/Language_Reference/The_Syntax_Language),
  and the semantics is typically implemented in Basic Storm, even though any language supported by
  Storm is usable.

- New definitions:

  Similarly to how it is possible to provide new syntax for code, it is also possible to provide new
  declarations, both at top-level and inside types. The process is similar to defining new syntax
  for functions: the extension defines syntax that instantiates relevant classes. In this case,
  however, the extension should eventually instantiate a named entity that will be added to the name
  tree. The process of adding entities to the name tree is coordinated by the loading process in
  Storm, and also by Basic Storm. As such, the process is a bit more involved when creating new
  definitions compared to new syntax inside functions.


This section will introduce some of the central concepts for creating extensions at a high level. As
this documentation aims to provide a high-level picture of the processes, the curious developer is
encouraged to explore the source code documentation of the classes in `lang:bs`, particularly
classes that inherit from `lang:bs:Expr`. There are also
[tutorials](md:/Getting_Started/Tutorials/New_Expressions) for getting started with developing
language extensions.
