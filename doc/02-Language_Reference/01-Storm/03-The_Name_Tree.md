The Name Tree
=============

The name tree is perhaps the most central component in Storm. As the name implies, the name tree is
a tree-like data structure that associates names with various elements in the system. The name tree
thus forms a global namespace that is shared between all languages in the system. The name tree is
also where Storm exposes its functionality to the languages in the system.


Basic Structure
---------------

The name tree is created from two central classes: `Named` and `NameSet`. The former, `Named`, is a
generic representation of something that has a name (a *named entity*). The latter, `NameSet`,
inherits from `Named`, and may additionaly contain zero or more named entities. As such, `NameSet`
allows creating nested namespaces. This is used to implement packages and types among other things.

Each instance of `Named` stores its own name. The name consists of a string, and zero or more
parameters. Each parameter is a reference to a type in the type system. Two entities are considered
to have the same name only if both the string and all parameters are equal. This allows overloading
names with different parameters. This is used to allow function overloading, and to implement
parameterized types (e.g. `Array`).

Finally, a note on terminology. The documentation uses *named entities* to refer to objects that are
reachable from the name tree. This term is simply used to highlight the fact that these objects have
a name, and that they are easily reachable from multiple languages. However, these *named entities*
are still regular instances of an actor type that inherits from `Named`.


Names
-----

Storm provides a representation of names to elements in the name tree. Similarly to how paths refer
to elements in the file system, a name in Storm consists of zero or more parts. Each part consists
of a string and zero or more type parameters, just like the name stored in the `Named` class. In the
standard textual representation, each part is denoted as `<name>(<parameter>, <parameter>, ...)`. If
no parameters are specified, the parentheses are omitted. Different parts are delimited by periods
(`.`). Below are some examples of names in the system:

```
core.Int                       // Name of the 32-bit integer type in Storm
core.Int.+(core.Int, core.Int) // Name of the addition operator in the Int type
core.Array(core.Int)           // Name of an array of integers
core.lang.rootPkg              // Name of the function to acquire the root package of the name tree
```

Note: The above representation is used by Storm itself. The notation used in Basic Storm is slightly
different for technical reasons. Namely, `:` is used instead of `.`, and `<>` are used instead of
`()`.

Storm provides two representations of names in non-textual forms. The first one is called a
`SimpleName`. A `SimpleName` is an array of `SimplePart`s, each consisting of a string and zero or
more type parameters. In the simple representation, all type parameters are pointers to type
entities in the name tree.

The other representation, `Name` is a sequence of `NamePart` objects. Each `NamePart` may be a
`SimplePart` as for `SimpleName`s, but it may also be a `RecPart`. The difference between a
`SimplePart` and a `RecPart` is that the parameters of a `RecPart` (for recursive part) are not
pointers to entities in the name tree, but instead unresolved names. The difference is thus that all
parameters in a `SimpleName` are resolved, while a `Name` may need recursive resolution of its
parameters.


Named entities
--------------

It is possible to store arbitrary data in the name tree by creating a class that inherits from
`Name` or `NameTree` as appropriate. Since the name tree is shared between different languages, this
is the primary mechanism which languages use to communicate with each other. To facilitate
interaction, Storm specifies a number of pre-defined entity types for common concepts, such as
functions and types. Languages that produce and consume entities that follow the Storm-specified
interfaces are thus able to interact freely. Languages are, however, not limited to using the
standard interfaces. They are free to create their own representations where the standard
representation is not enough. This will, however, mean that other languages are likely to interact
with those features of the language.

Some of these Storm-provided named entities that are of particular importance are as follows:

- `core.lang.Package`

  This entity inherits from the `NameSet` entity mentioned above and implements the concept of
  packages. In Storm, each package corresponds to a directory in the file system. The root of the
  name tree is a package that corresponds to the `root/` directory in the file system. The `Package`
  class is then responsible for creating `Package` entities for all subdirectories, and loading
  source code as appropriate. How source files are loaded is described [here](md:Files_and_Packages).

- `core.lang.Function`:

  This entity inherits from `Named` and represents a callable function. The parameters of the
  `Named` are expected to correspond to the formal parameters of the function. This means that it is
  possible to store multiple functions with the same name, as long as they have different parameter
  lists. This ability is thus used to implement function overloading.

- `core.lang.NamedThread`:

  This entity inherits from `Named` and represents a named thread, as understood by the type system.
  The entity essentially encapsulates a single OS Thread (`core.Thread`).

- `core.lang.Variable`:

  This entity encapsulates the concept of a variable. By itself it is not very useful, but Storm
  provides two subclasses for interoperability: `GlobalVar`, and `MemberVar`. The former represents
  a global variables, and the latter represents a member variable of a type.

- `core.lang.Type`:

  This entity inherits from the `NameSet` entity mentioned above and implements types in Storm. As
  such, the type system is exposed to languages through this class. Each instance of the entity
  corresponds to a type in the [type system](md:Type_System) (properties in the class determine what
  kind of type it is).

  Since `Type` inherits from `NameSet`, it may contain other entities. While any named entity can
  technically be stored inside a `Type`, functions and variables are of special interest to the
  `Type` entity. Entities of the type `MemberVar` are used to represent member variables, and the
  `Type` entity uses them to compute the in-memory representation of instances of the class.
  Entities of the type `Function` constitute member functions. The `Type` entity keeps track of them
  to determine which functions require virtual dispatch. Note that non-static member functions are
  required to have a `this` pointer as an explicit first parameter.


Name Lookup
-----------

Storm provides a customizable way of traversing the name tree to resolve names into named entities.
This process is encapsulated by the type `Scope`. A scope consists of a pointer to a `NameLookup`
and a `ScopeLookup`. The `NameLookup` determines the "current" location in the name tree (similarly
to the working directory in the terminal). The `Named` class inherit from `NameLookup`, so all
`Named` entities can be used for this purpose. The separation between `Named` and `NameLookup`
allows the creation of anonymous scopes (e.g. blocks in imperative languages) that can be used with
the standard name lookup. The `ScopeLookup` specifies a *strategy* for resolving names. The strategy
dictates how the `NameLookup` should be used when attempting to resolve a name. For example, the
`ScopeLookup` might decide to attempt to resolve names in all parent `NameSets`, or it might decide
to only consider `Types`, the first package, and then a set of explicitly included packages. This
mechanism means that a language is able to customize the logic behind the name lookup to a high
degree, and to even share its name resolution logic when interacting with other languages. Since the
"current location" is separated from the lookup logic, it is also easy to modify the "current
location" while retaining the name resolution logic.


Visibility
-----------

Each `Named` object may optionally be associated with a *visibility*. The visibility determines
which parts of the system is allowed to access a particular `Named`. This mechanism is used to
implement access controls, such as `public` and `private`, but allows more flexibility if desired.

The visibility of a `Named` object is represented by instances of subclasses to the `Visibility`
class. This class is not much more than a predicate function, `visible(Named check, NameLookup
source)` that determines if the entity `check` is visible from the `source`. This function is called
from the function `Named.visibleFrom` during name lookup to determine visibility. There are a couple
of default implementations for common keywords provided by Storm. For example, the classes `Public`,
`TypePrivate`, `TypeProtected`, `PackagePrivate`, etc.

The visibility is assessed during type lookup. Thus, it is necessary to provide a `Scope` that
describes the caller's scope when calling `NameLookup.find()`. It is also possible to override the
visibility system by providing an empty scope (i.e. `Scope()`) to the `find` function. If a `Named`
object is not associated with a `Visibility` object, Storm considers it to be visible from everywhere.
