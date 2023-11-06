Named Entities
==============

This page provides more detail on the central named entities that are provided by Storm, and how
they are used. As with most entities in the name tree, these entities can be extended by other
languages to provide additional functionality. Other languages may, however, not be able to
understand the additional information. As a concrete example, Basic Storm is not able to understand
the syntax from the Syntax Language, since the Syntax Language adds such information in a subclass
to the `Type` entity that Basic Storm is not aware of.

The name tree is built from named entities. The name tree is a linked structure with pointers both
from the root to its leaves, but also from the leaves towards the root. The former does not always
exactly match the latter. For example, it is typically desirable to be able to perform type lookups
inside blocks in functions. As such, the blocks have a pointer to its parent in the tree to allow
traversing it towards the root to inspect the static context. However, it is typically not
interesting for individual blocks inside a function to be reachable from the root (they are
anonymous). As such, languages typically only adds a pointer to entire functions to the name tree,
but functions themselves do not contain pointers to all the contained blocks.


NameLookup
----------

A [stormname:core.lang.NameLookup] represents something that can act as a starting point for name
resolution. The `NameLookup` is itself not a named entity as it has no name. It is, however, one of
the base classes for named entities in the system.

To accommodate name lookups, the `NameLookup` contains the following two members:

```stormdoc
@core.lang.NameLookup
- .find(core.lang.SimplePart, core.lang.Scope)
- .parentLookup
```

Users of `NameLookup` are encouraged to use the `parent()` function instead of the `parentLookup`
variable directly. The default behavior of the `parent()` function is to assert whenever
`parentLookup` is not set, as this typically indicates that a named entity was not properly added to
the name tree. Implementations may, however, override `parent` to return `null` to indicate that the
root of the name tree has been reached (e.g. for the root package).

As mentioned above it is, however, possible to set `parentLookup` manually to create entities that
are not reachable from the root of the name tree (and thus not reachable by most parts of the
system), but that may participate in name resolution. Typical examples of this is blocks inside of
functions in a programming language. These are anonymous, so there is no value in making them
accessible from the name tree. They do, however, contain variables, so it is useful to utilize the
name resolution system already in Storm to lookup local and non-local variables.

In addition, the `NameLookup` class contains a number of convenience overloads for `find`.


LookupPos
---------

The type [stormname:core.lang.LookupPos] is an intermediate class between `NameLookup` and `Named`.
It adds a source position to indicate the current location. This is mainly used to implement
file-private visibility. To determine if an entity is accessed from the same file, the visibilty
check examines if a `LookupPos` is in the chain of `parent` pointers in order to determine the
current source file.


Named
-----

The [stormname:core.lang.Named] is the base for all named entities in the system. It inherits from
`LookupPos`, and thereby indirectly from `NameLookup`. This means that it inherits a pointer to a
parent entity in the name tree.

Apart from the parent entity, the `Named` type also contains a name. The name consists of two parts,
a string and a list of zero or more parameters. It is stored as two members as follows:

- [stormname:core.lang.Named.name]

  A string that represents the name of the entity.

- [stormname:core.lang.Named.params]

  An array of [stormname:core.lang.Value] that represents parameters to this entity. Each `Value` is
  essentially a reference to a type (but with some addtitional metadata, see below). Parameters are
  used to disambiguate different versions of the entity, for example overloaded functions and arrays
  that contain different types.

The named entity also contains the member `flags` that indicate if the named entity should be
treated specially during name lookup. Currently, the only flag that is available is
`NamedFlags:namedMatchNoInheritance`, which specifies that inheritance should not be considered when
matching parameters. This is typically desirable for templated types.

Apart from the name and flags, a named entity may also have a visibility. The visibility is
represented as an instance of a subclass of [stormname:core.lang.Visibility] that is stored in the
`visibility` member. If the `visibility` member is `null`, then the entity is considered to be
visible from anywhere in the system. The utility function `visibleFrom` checks visibility using the
`visibility` member.

Finally, the named entity may have an attached documentation entity in the `documentation` member.
The function `findDoc` can be used to retrieve the documentation conveniently.

The following convenience functions are also available:

```stormdoc
@core.lang.Named
- .path()
- .identifier()
- .shortIdentifier()
- .compile()
- .discardSource()
```

Function
--------

The [stormname:core.lang.Function] entity is a named entity that represents a function. The
parameters of the function entity are assumed to directly correspond to the formal parameters
accepted by the function, including any implicit `this` parameters. The function also has a set of
flags, marking it as *pure*, *autocast*, *final*, *override*, or *static*. The *pure* flag means
that the system may assume that the function does not inspect nor modify any state outside of its
parameters. Calls to pure functions may thereby be optimized away if desired.

Note that functions that correspond to constructors (named `__init`) are typically not possible to
call directly.

A function contains one instance of the [stormname:core.lang.Code] class that contains the actualy
code that should be executed by the function. Apart from that, the function contains two
`RefSource`s that other parts of the system may refer to. One `RefSource` always refers directly to
the `Code` in the function, while the other may be modified to instead refer to any logic required
for dynamic dispatch. Having two separate `RefSources` means that call-sites can specify if the
dispatch logic should be called or not.

A function contains the following members:

```stormdoc
@core.lang.Function
- .result
- .ref()
- .directRef()
- .isMember()
- .runOn()
- .pure()
- .setCode(*)
- .getCode()
- .getLookup()
- .setLookup(*)
- .compile()
- core.lang.pointer(*)
```

The `Function` class also contain a number of functions to automatically generate code for calling
the function. These are a number of variants, each with a number of overloads:

- `localCall`

  Generate code to call the function from a context where it is known the caller executes on the
  same thread as the callee.

- `threadCall`

  Generate code to call the function from a context where it is known that a message needs to be
  sent to another thread.

- `autoCall`

  Calls either `localCall` or `threadCall` depending on the provided context.

- `asyncLocalCall`

  Generate code for an async call (i.e. returning a `Future`) for cases where the caller executes on
  the same thread as the callee.

- `asyncThreadCall`

  Generate code for an async call (i.e. returning a `Future`) for cases where a message needs to be
  send to another thread.

- `asyncAutoCall`

  Calls either `asyncLocalCall` or `asyncThreadCall` depending on the provided context.


### Code

The type [stormname:core.lang.Code] is the base class to a hierarchy of types for different
situations. The ones most relevant when generating code from Storm are the followin ones:


- [stormname:core.lang.DynamicCode]

  Generates code from a [stormname:core.asm.Listing] object and allows executing it.

- [stormname:core.lang.LazyCode]

  Generates a stub that calls the supplied function to generate the actual body of the function the
  first time it is executed. This is how lazy loading is achieved in large parts of Storm.

- [stormname:core.lang.InlineCode]

  Dynamically generated code that may be inlined elsewhere. The `InlineCode` class takes a callback
  function that is called to generate code. The function callback receives an `InlineParams` object
  that contains the state of code generation, and information about parameters. The `InlineCode`
  class also generates a stand-alone version of the code to support creating function pointers to
  it, for example.


Variable
--------

The [stormname:core.lang.Variable] is a generic representation of a variable. It only has one
additional member, namely: `type` that stores the type of the variable.


GlobalVar
---------

The [stormname:core.lang.GlobalVar] entity inherits from [stormname:core.lang.Variable] and
represents a global variable. Global variables are expected to only be accessible from a particular
named thread to ensure thread safety. The global variable is lazily initialized by providing a
function pointer that contains the initialization logic.

MemberVar
---------

A [stormname:core.lang.MemberVar] entity is a variable that is a member of a type. As such, it does
not provide any storage for the actual variable itself, but cooperates with the `Type` entity to
allocate storage in the type.


NamedThread
-----------

The [stormname:core.lang.NamedThread] entity represents a named thread in the system. The
`NamedThread` entity is essentially a thin wrapper around a `Thread` object, that also contains a
name and allows accessing it easily in generated code through a reference.


NameSet
-------

The [stormname:core.lang.NameSet] entity is a named entity that may contain zero or more other named
entities. It is thus the type responsible for the central nature of the name tree. As the name
implies, all entities stored inside of it need to have unique names.

The `NameSet` provides the following members for inspecting and manipulating its content:

```stormdoc
@core.lang.NameSet
- .add(Named)
- .remove(Named)
- .anonName()
- .content()
- .find(*)
- .findName(*)
- .begin()
- .end()
```

Apart from storing entities, the `NameSet` provides two mechanisms for dynamically generating named
entities on demand. The first one is through a mechanism called *templates*. A *template* in Storm
is essentially just a callback with a name without parameters. When a template is inserted into a
`NameSet`, then the `NameSet` will execute the callbacks in all templates associated with the name
that is beeing looked up whenever it does not already contain a suitable named entity. The templates
are thus free to inspect the parameters and determine if they wish to generate a suitable named
entity on demand or not. If they do, the named entity is inserted into the name set and used for
future lookups.

The functions used to manipulate templates are as follows:

```stormdoc
@core.lang.NameSet
- .add(Template)
- .remove(Template)
```

The second mechanism is lazy-loading. This mechanism relies on overloading protected members, and
therefore it requires subclassing the `NameSet` entity. A name set is initially in the *unloaded*
state. This means that whenever it fails to find a named entity (after template lookup), it will
attempt to load its content. This occurs in two steps. In the first step, the `NameSet` calls the
function `loadName` to attempt to load only the requested name. If it is successful, the name set
concludes its search and remains in the *unleaded* state. If loading of a single entity fails (e.g.
because it is not supported), then the `loadAll` function is called instead. This instructs the
derived class to load all entities into the `NameSet`. After this, the `NameSet` switches to the
*loaded* state, which means that it will no longer attempt lazy loading.

In most cases, lazy loading is transparent to the user of the interface, as the `NameSet` will
handle loading transparently. The exception is, however, iterating through the `NameSet`. Doing this
will *not* cause automatic loading, as that would make it impossible to traverse the currently
loaded parts of the name tree. Instead, the `forceLoad` member is available to force the `NameSet`
to load all its content early.

The following members are used for lazy loading:

```stormdoc
@core.lang.NameSet
- .loadName(*)
- .loadAll()
- .allLoaded()
- .tryFind(SimplePart, Scope)
```

The default implementation of the `loadName` and `loadAll` functions above simply return `false` and
`true` respectively. This means that lazy-loading will succeed, but not load anything if they are
not overridden. This means that it is possible to ignore lazy-loading unless it is needed.


Package
-------

The [stormname:core.lang.Package] extends [stormname:core.lang.NameSet] and represents a package in
the system. Packages generally correspond to directories in the file system, but it is also possible
to create *virtual packages* that do not exist on disk. Virtual packages must therefore be populated
manually.

The package entity add two main things to the interface of `NameSet`: automatic discarding of source
code and exports. By default, a package will automatically ask all loaded entities to discard
references to source code after they are loaded. This means that it is not possible to ask functions
to re-generate intermediate code once they are created, for example. This is usually desired, as it
is only necessary to generate code for each function exactly once. In some cases, however, it is
desirable to inspect and modify the generated code. In such cases, the automatic discarding needs to
be disabled by calling `noDiscard`.

The other concept, exports, allows a package to specify that other packages should be included
alongside it. For example, if a language extension in package `a` adds to the syntax in `b`, then
`a` will not work unless `b` is also included. To avoid confusion, `a` can express this dependency
by *exporting* `b`. This means that including `a` from a language implicitly also includes `b`.

The additional interface consists of the following members:

```stormdoc
@core.lang.Package
- .url
- .noDiscard()
- .export(*)
- .exports()
- .recursiveExports()
```


Type
----

The entity [stormname:core.lang.Type] also extends [stormname:core.lang.NameSet]. It represents a
type in the type system. As such, it extends the interface of `NameSet` with functionality for
managing inheritance, object layout, for example. The `MemberVar` entity (see above) is specially
recognized by the `Type` entity. They collaborate to provide object layout.

What kind of type is represented is set on construction through the `TypeFlags` parameter. It can
also be retrieved using the `typeFlags` member. The `Value` class can also be used to inspect types
conveniently.

As with many other parts of Storm, the `Type` class generates its contents lazily. This means that
it is possible to modify the contents of a type until the first instance is created. At that point,
the object layout is finalized, and members may no longer be added or removed. This restriction
does, however, not apply to functions that do not rely on virtual dispatch, or other named entities.

The `Type` entity provides the following additional member functions:

```stormdoc
@core.lang.Type
- .abstract()
- .setSuper(*)
- .super
- .declaredSuper
- .chain
- .setThread(*)
- .runOn()
- .variables()
- .vtable()
- .size()
- .typeRef()
- .typeDesc()
```

