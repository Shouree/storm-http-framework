Names
======

Most things in the compiler has a name. Therefore, it is good to know how name resolution works
in the compiler. Storm provides ways to alter the way name lookups are performed on a case-by-case
basis. Therefore, this section mostly describes the framework as well as the expected way to look up
names.

A name is a sequence of parts, much like a path in your file system. In a file system each part only
contains a name, but in Storm, each part also contains zero or more type parameters. These type
parameters are direct references to a type in the type system, so no arbitrary strings or values are
allowed as parameters to parts. This does not make much sense if we only consider packages (which
usually does not take any parameters), but if we also consider template types and functions, it
starts making sense. In this way, a name is either an absolute or relative path to something in the
type system.

Everything in the type system inherits from the class `core.lang.Named`. A `Named` contains, just
like each part of a name, a string and zero or more type parameters. `Named` also inherits from
`NameLookup`, which is an interface indicating that it is possible to look up names relative to that
point in the type system. This means, that given a name and an entry point, we can traverse the
`Name` hierarchy to find out what the name resolves to.

This process works as long as all names are relative to some fixed point. However, this often
becomes cumbersome since we either have to specify full names of everything we are using, or we have
to restrict ourselves to only a part of the type system (there is nothing that is equivalent to `..`
in names). Therefore, Storm has something that is called a Scope.

A `Scope` is an entry point into the type hierarchy along with a policy of how we want to traverse
the type system. This policy might be: traverse from the entry point, if that fails, assume the name
is an absolute name and traverse from the root of the type system. This can also be extended to look
at any includes as well, or to do any number of interesting things. Using this mechanism, it is
possible for one language to "leak" its name resolution semantics into other languages. This may be
good or confusing, depending on how it is done. Consider, for example, embedding languages into each
other. In this case it makes sense in some cases that the embedded language follows the name
resolution of the top-level language. In other cases it does not.

As mentioned earlier, the parameters present at each part of a name are used to implement function
overloading and templates. This functionality is implemented by the class `NameSet`, which is
basically a collection of different names. The `NameSet` enforces that names does not collide with
each other (considering parameter), and implements how to resolve parameters. By default, `NameSet`
considers inheritance when examining which overloads are suitable. This can, however, be disabled on
a overload-by-overload basis if desired. `NameSet` also implements support for templates. A
`Template` is an object that can generate `Named` objects on demand. Whenever the `NameSet` is
requested for a name with parameters it does not find a match for, it asks a `Template` with the
same name (if it is present) to generate the match.

Member functions and variables always have an explicit this pointer as their first parameter.


Visibility
-----------

Each `Named` object is optionally associated with a *visibility*. The visibility determines which
parts of the system is allowed to access a particular `Named`. This mechanism is used to implement
access controls, such as `public` and `private`, in Storm, but allows more flexibility if desired.

The visibility of a `Named` object is represented by instances of subclasses to the `Visibility`
class. This class is not much more than a predicate function, `visible(Named check, NameLookup source)`
that is called by `Named.visibleFrom` whenever Storm needs to determine if the object `check` is visible
from `source`. There are a couple of default implementations for common keywords provided by default, for
example `Public`, `TypePrivate`, `TypeProtected`, `PackagePrivate`, etc.

The visibility is assessed during type lookup. Thus, it is necessary to provide a `Scope` that
describes the caller's scope when calling `NameLookup.find()`. It is also possible to override the
visibility system by providing an empty scope (i.e. `Scope()`) to the `find` function. If a `Named`
object is not associated with a `Visibility` object, Storm considers it to be visible from everywhere.
