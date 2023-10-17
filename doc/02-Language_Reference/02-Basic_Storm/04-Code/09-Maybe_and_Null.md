Maybe and Null
==============

Variables may not contain the value `null` by default. This is true even for class- and actor types
that are represented by a reference that could technically contain `null`. Instead, Basic Storm
requires using the type `Maybe<T>`, or its shorthand `T?`, to represent a type that may contain
either the type `T` or the value `null`. Since `Maybe<T>` is a separate type, it is possible to
create `Maybe<T>` for all `T` in the system, including types that are not reference-types.

The main benefit of this approach is that the language is aware of all variables that may be `null`,
and can require the programmer to explicitly check them at appropriate points. In particular, the
class `Maybe<T>` only contains members for checking if an element is present (`empty` or `any`).
There is no way to access the element inside the `Maybe` type.

Instead, elements in a `Maybe<T>`-type must first be casted into a non-maybe type, `T`, by using a
*weak cast*. Weak casts are described in full in the section on [type
conversions](md:Type_Conversions), but a brief example is provided below for completeness.

To check whether a `Maybe` type contains an object, one can simply use the `any` or `empty` members
of the `Maybe` type as follows:

```bs
void fn(Int? x) {
    if (x.any) {
        // 'x' was not null.
        // We can still not access its contents...
    }
}
```

Using a *weak cast*, we are also able to access the element if it was not null:

```bs
void fn(Int? x) {
    if (v = x) {
        // 'x' was not null.
        print(v.toS); // We can access it now!
    }
}
```

In this particular case, we could simply write `if (x)`, and use `x` in the body of the `if`
statement. More details are available in the [type conversions](md:Type_Conversions) section.

Basic Storm is able to automatically convert from non-`Maybe` types into `Maybe` types in many
cases. For example, the following function will work as expected:

```bs
Str? fn() {
    "fn"; // Automatically casted to Str?
}
```

In some situations, it might be necessary to manually cast to `Maybe<T>`. This can be done either by
calling the `Maybe<T>` constructor, or by using the `?` prefix operator. One example where automatic
casts fail currently in is inside if-statements:

```bs
Str? fun(Bool c) {
    if (c) {
        fn(); // calls 'fn' from the above example.
    } else {
        "fun";
    }
}
```

The code above will fail to compile since the `if` statement does not consider `Maybe<Str>` and
`Str` to be related. The issue can be solved either by explicitly returning the results with the
`return` statement. It might also be solved by an explicit cast in the second case:

```bs
Str? fun(Bool c) {
    if (c) {
        fn();
    } else {
        ?"fun"; // or Str?("fun")
    }
}
```


Null
----

Variables of the type `Maybe<T>` are initialized to `null`. Basic Storm also provides the keyword
`null` as a shorthand for `Maybe<T>()` or `T?()`. The `null` keyword is a bit special. Since it is
not possible to infer the exact type of a `null` value (i.e. the type is `Maybe<T>` for *any* `T`),
it needs to be used in a context where Basic Storm can infer the type. For example, the following
situation works as expected since the function needs to produce a `Maybe<Int>` as a result:

```bs
Int? fn() {
    return null;
}
```

However, the following would produce an error, since we asked Basic Storm to infer the type of the
variable:

```bs
void fn() {
    var x = null; // Will prouce an error.
}
```


Representation of `Maybe<T>`
--------------

The `Maybe<T>` has two implementations, depending on whether `T` is a value, or if it is a class or
an actor. For classes and actors, `Maybe<T>` simply contains a reference to the object. In contrast
to other references in the system, the one stored inside `Maybe<T>` might be `null`. For value
types, `Maybe<T>` stores a copy of the value. After the value, it stores a boolean value that
indicates whether or not it represents `null`. As such, `Maybe<T>` for classes does not incur any
extra memory, while `Maybe<T>` for integers incurs an extra boolean of overhead (which typically
consumes 4 or 8 bytes due to alignment requirements).
