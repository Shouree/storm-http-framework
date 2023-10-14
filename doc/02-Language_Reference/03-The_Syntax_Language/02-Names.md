Names in the Syntax Language
============================

Names in the Syntax Language are similar to the [generic syntax](md:../Storm/Names) in Storm. The
only difference is that angle brackets (`<>`) are used to specify parameters to name parts instead
of parentheses (`()`). Parentheses are only used for function (or rule) calls.

As such, names in the Syntax Language are written as:

```
a.b<c>.d
```

Note that it is not possible to combine parameters in angle brackets with parameters in parentheses.
This is because they are both used to denote the parameters of the associated name part. As such,
the following is not allowed:

```
fn<Int>(a);
```

Since both `<Int>` and `(a)` describes the parameters to the part `fn`. The exception to this rule
is when the name denotes a type. Then the parameters in angle brackets refer to the type, and the
parameters in parentheses refer to the parameters to the constructor. As such, the following is
legal:

```
core.Array<core.Int>(x);
```

It is equivalent to the name `core.Array<core.Int>.__ctor(x)` (it is, however, not possible to call
constructors explicitly).


Compared to [Basic Storm](md:../Basic_Storm/Names), there are no short-hand names for common types.
As such, types like `Array<T>`, `Maybe<T>`, or `Map<K, V>` must be spelled out.


Name Lookup
-----------

Name lookup in the Syntax Language is comparatively simple due to the limited expressivity of the
language. There are three situations in which names can occur. They are as follows:

1. To refer to types or rules. In this case, the Syntax Language constructs a `Name` that
   corresponds to the name and gives it to the standard lookup in Storm.

2. As a function call in a production. In this case, the name is a function- or constructor call if
   it ends with parentheses, or a variable name otherwise. If it is a function call, the [Basic
   Storm lookup](md:../Basic_Storm/Names) is used. However, since all parameters have to appear in
   the parentheses, it can be summarized as follows:

   First, the Syntax Language attempts to find the name using a standard lookup (taking into account
   the fact that it might refer to a constructor). Otherwise, it attempts to resolve the name in the
   context of the type of the first parameter, in case it refers to a member function.

3. As a target for a match inside a production (i.e. `<item> -> <name>`). In this case, the name
   must consist of a single part, and have no additional parameters. The name is treated as if one
   had written `me.<name>(<param>)` in Basic Storm. As such, the name is first resolved in the
   context of whatever type `me` refers to in the current context, and then according to the
   standard name resolution rules.

The standard name resolution rules in the Syntax Languages are similar to those of Basic Storm.
Except for the special case of examining the type of the first parameter when noted above, names are
resolved in the following order:

1. the current package
2. any `use`d packages
3. the core package
4. the root package

One special case exists: the type `core.lang.SStr` is always visible, even if the `core.lang`
package has not been included.
