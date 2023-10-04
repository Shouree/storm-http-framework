Storm
=====

Storm itself is a language-agnostic platform for creating extensible programming languages that are
able to interact with each other. To allow languages to interact with each other, Storm provides an
extensible type system, and defines interfaces for specifying a number of common concepts. These
interfaces serve two purposes. First, they give access to the functionality provided by the runtime,
and secondly they provide a means for languages to expose their functionality to other languages in
a uniform way.

The two central parts of this interoperability are the *type system* and the *name tree*. The type
system provides a generalized way to express data structures along with their associated operations.
The name tree then provides a way to organize data structures and operations into namespaces, so
that it is possible to associate names to them.

The remainder of this section thus covers these concepts in a language-agnostic way. Since they are
the building blocks for other languages in the system, they constitute the fundamentals of other
languages in the system. This is particularly true for Basic Storm and The Syntax Language, as they
are designed to closely match the underlying concepts from Storm.

This section covers the following topics:

- [Threading Model](md:Threading_Model) covers the way Storm handles threads. The type system is
  aware of the threads in the system, and aims to ensure that data races are not possible.

- [Type System](md:Type_System) covers the type system in Storm. The type system is used to
  define all other interfaces in Storm, and is thus a central part of Storm.

- [The Name Tree](md:The_Name_Tree) introduces how types, functions, and other entities are
  organized into namespaces to uniquely name things in Storm. This section also describes how the
  name tree can be traversed and extended by language implementations.

- [Exceptions](md:Exceptions) introduces how exceptions work in Storm in general. It also lists some
  of the most important parts of the exception hierarchy in Storm.

- [Compilation Model](md:Compilation_Model) describes how code is compiled in Storm. Since Storm is
  fairly unique in utilizing *lazy compilation*, this can lead to surprises for developers that are
  familiar with both languages that are compiled ahead of time and dynamic languages, since Storm
  behaves as if it were somewhere between the two.

- [Packages and Files](md:Packages_and_Files) describes the relation between packages and the
  filesystem. Of particular importance is the process for determining which language to use when
  reading different filetypes in the file system.

- [Top Loop](md:Top_Loop) introduces how top loops (REPLs) work in Storm, and how to implement one
  for a new language.

- [Language Server](md:Language_Server) introduces the language server in Storm. The language server
  is able to provide syntax highlighting of Storm programs to editors, and to further integrate
  Storm into your favourite text editor or IDE.


This part of the manual aims to provide an overview of high-level concepts. As such, the
documentation typically mentions types that are responsible for certain actions, but does not
provide details on their exact usage or interface. For details on particular interfaces, the reader
is referred to the [built-in documentation](md:/Getting_Started/Running_Storm/Help) that is
available by typing `help <type name>` at the interactive top loop.
