Name lookup
============

This page describes the semantics of name lookup in Basic Storm. In general, the idea is that names
that are near the usage have precedence over names that are lexically far away. Apart from this, the
name lookup aims to be as intuitive as possible while keeping it simple.

Basic Storm uses the following syntax for names:

```
a:b<c:d>:e
```

Where the `<>` are used to indicate parameters to that specific part (see
[Names](md://Storm/Names) for an explanation of names in Storm). Also note that the `.`
operator is not used to separate names. This operator is only used when accessing members of a
value, much like in C++ (using the `::` operator).


Name lookup rules
-----------------

Usage of names in Basic Storm has the following form in general: `a.N<T, U>(b, c)`. The part `N` is
an entire name with parts separated by `:`. `T` and `U` are type names, and `a`, `b`, and `c`
are arbitrary expressions. All parts except `N` are optional, so the following are also valid: `N`,
`a.N`, `N(b, c)`, etc.

Name lookup proceeds as follows:

1. If parameters in angle brackets (`T`, `U`) are present, then the name `N` has to refer to a type
   with matching template parameters. The parameters in parenthesis are then parameters to the
   constructor of the type. Example: `core:Array<Int>(1, 0)`.
2. If the name has the form `a.N(...)` (with or without parameters), then the name is interpreted as
   `N(a, ...)`, and name lookup proceeds as usual. The term `a` is remembered to be "scope-altering"
   for future name tree traversal.
3. If the name tree did not have the form `a.N(...)` originally, and the name occurs in a context
   where a variable named `this` exists (typically inside member functions), then two options are
   created for the name. One with the form `N(this, ...)`, and one with the original form
   (i.e. `N(...)`). Both are resolved, but the original form is resolved first at each step.

This process produces one or two names that need to be resolved in the name tree. Regardless of
which path was taken above, traversal of the name tree proceeds as follows:

1. If `N` consists of only one part, and the first parameter was marked as "scope-altering", look
   inside the type of the first parameter for a match. This means that expressions like `x.y()`
   always resolve in the context of the type of `x` if it exists.
2. Traverse the name tree from the current local scope, until the current package is reached. At
   each step, attempt to resolve `N` (with the current parameters).
3. If `N` consists of only one part, and the first parameter was not marked as "scope-altering",
   look inside the type of the first parameter for a match. This means that expressions like `y(x)`
   are allowed to match against member functions in the type of `y`, but that function calls closer
   have priority.
4. Attempt to resolve `N` against the root package.
5. Attempt to resolve `N` against each `use` statement in the current file.


Use statements
---------------

Each Basic Storm file may start with zero or more `use` statements. These make the names in the use
statements accessible in the current file as above. In addition to affecting the name resolution,
they also import syntax from the specified packages. This means that if a package (e.g. `ui`)
provides custom syntax, importing it with `use` statements makes it possible to use that syntax.

