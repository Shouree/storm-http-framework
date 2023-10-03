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

- *Syntax*: General syntax.
