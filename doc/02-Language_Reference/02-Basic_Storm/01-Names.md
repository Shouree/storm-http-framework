Names in Basic Storm
====================

Names in Basic Storm are similar to the [generic syntax](md:../Storm/Names) of Storm. The syntax of
names are, however, altered slightly to allow using the dot `.` operator for member access, like C++
and Java. The difference from the generic Storm syntax is as follows:

- The dot `.` has been replaced by a colon `:` (inspired by the `::` operator in C++).
- Parentheses `()` have been replaced by angle brackets `<>`.

This means that names in Basic Storm generally have the following form:

```
a:b<c:d>:e
```

The rationale behind these differences is to distinguish the *member access* operator (`.`) from the
*scope resolution operator* (`:`). Basic Storm uses `.` as the member access operator and `:` as the
scope resolution operator to resemble the syntax of languages like C++. Similarly, since the name
syntax is most often used to refer to the name of types, angle brackets (`<>`) were used instead of
parentheses (`()`) to distinguish type parameters from function parameters.

*Historical note*: The original plan was for Basic Storm to overload the dot operator (`.`) like
 Java, so that its meaning would depend on the context in which it was used. This has not yet
 happened.

To illustrate the syntax, consider the following examples:

- `core:Int` is the name of the `Int` type in the `core` package.
- `core:Array<core:Int>` is the name of the `Array` type for the `Int` type.


Short-hand Names
----------------

Basic Storm provides a number of short-hand names for common types as follows:

- `T[]` is short for `Array<T>` (an array of `T`)
- `T?` is short for `Maybe<T>` (either `T` or `null`)
- `K->V` is short for `Map<K, V>` (a hash map from `K` to `V`)
- `fn(...)->T` is short for `Fn<T, ...>` (a function with parameters `...` and return value `T`).
  Both of `...` and `->T` may be omitted.

Additional type aliases can be added by providing additional productions for the rule
`lang.bs.SType`.


Use Statements
--------------

Each Basic Storm file may start with zero or more `use` statements. A use statement specifies a
package to "import" into the current source file. For example: `use my:package;`. A `use` statement
serves two purposes. First, they make all names inside the specified package visible in the current
file without qualification. For example, if `use lang:bs;` is present, then the name `Expr` can
refer to the type `lang:bs:Expr`. Secondly, use statements import syntax from the specified package.
The imported syntax is available in the remainder of the file, but only after all `use` statements.


Name Lookup
-----------

A name in Basic Storm may appear in two contexts. The first, and simplest, is when a name appears in
a location where a type name appears. In this case, the name is resolved as-is after any short-hand
names have been expanded. A name may also appear in an expression, possibly with parameters and in
conjunction with the member access operator. In general, such a name may have the following form:

```
a.N<T, U, ...>(b, c, ...)
```

In the generic form, `N`, `T` and `U` are fully qualified names according to the syntax above (i.e.
parts separated with `:` and any parameters specified with `<>`). The parameters to the last part of
`N` (if any) are `T`, `U`, and optionally more parameters. Of cours, `N` may contain multiple parts.
However, the parameters of the last part are important for the name lookup, which is why they are
explicit in the form above. Furthermore, `a`, `b`, and `c` are expressions. The exact syntax of
expressions are covered later. What is important here is that all expressions have a type that is
known at compile-time. This type is used during name lookup. All parts of the generic form are
optional except for `N`. This means that the following forms are also valid: `N`, `a.N`, `N(a, b, ...)`, etc.

Name lookup then consists of two steps. First, the system decides on one or two names that should be
resolved in the name tree as follows:

1. If the last part of `N` contain any parameters (i.e. `<T, U, ...>` above), then the name `N<T, U,
   ...>` has to refer to a type. Any parameters in parentheses are then parameters to the
   constructor of the type. For example, the name `core:Array<Int>(1, 0)` refers to the constructor
   of the type `core:Array<core:Int>`. *Note*: The constructor is named `__init` with suitable
   parameters. It is, however, not possible to explicitly call the constructor by name from Basic
   Storm.

2. If the name has the form `a.N(...)`, with or without parameters in parentheses, then the name is
   interpreted as `N(a, ...)`, and name lookup proceeds as usual. The term `a` is remembered to be
   *scope-altering* for the future scope traversal. The last part of `N` is appended with the
   parameters `a`, `b`, `c`, ...

3. If the name did not have the form `a.N(...)` originally, and the name occurs in a context where a
   variable named `this` exists (for example, in a member function), then two versions of the name
   are created. The first with parameters the type of `this`, `a`, `b`, ... and the second with
   parameters `a`, `b`, ... (These correspond to `N(this, a, b, ...)` and `N(a, b)`). Both are
   resolved, but the original form takes priority at each step.

After the interpretation of the name has been determined, the name tree is traversed as follows;

1. If `N` consists of only one part, and the first parameter was marked as *scope altering* (step 2
   above), look inside the type of the first parameter for a match.

   This means that expressions like `x.N()` always resolve in the type of `x` if it exists.

2. Traverse the name tree from the current local scope until a package is reached. At each step,
   attempt to resolve `N` with its current parameters.

   This means that names are resolved in the lexical context of the current file and package first.
   However, since this step ends at the current package, a name like `a:b` that occurs in the
   package `c:d` will not be able to match `c:a:b`, which would likely lead to confusion.

3. If `N` consists of only one part, and the first parameter was not marked as *scope altering*,
   look inside the type of the first parameter for a match.

   This means that expressions like `N(x)` are allowed to match against member functions in the type
   of `x`, but that function calls named `N` declared lexically closer have precedence when this
   form is used. Compare this to step 2, which applies to names of the form `x.N()`.

4. Attempt to resolve `N` against the root package.

   This means that absolute names are matched at this stage, and that they take precedence over any
   `use` statements.

5. Attempt to resolve `N` against the package `core`.

   This makes all types in the `core` package (e.g. `core:Int`, `core:Str`) automatically visible
   everywhere.

5. Attempt to resolve `N` against each `use` statement in the current file.

   This means that `use` statements are used as a *last restort* for name resolution.
