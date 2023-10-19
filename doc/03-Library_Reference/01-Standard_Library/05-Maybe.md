Maybe
=====

The type `Maybe<T>` represents either an instance of type `T`, or the absence of an instance,
`null`. `T` may be any type in the system.

This type is central to the system, as it allows explicitly marking which variables may contain
null, and which are guaranteed to always contain something. As such, the type has a special syntax
in Basic Storm: `T?`.

For cases where `T` is an object or an actor, the type `Maybe<T>` does not introduce any overhead
memory-wise. It simply stores the reference to the object or actor, and uses the special value of
zero to represent a `Maybe<T>` that contains `null`. For values, `Maybe<T>` is not able to use a
special representation to represent `null`, but has to add one byte to the end of the value to
indicate whether or not the contained representation is valid.

The type `Maybe<T>` is always a value, but if `T` is an object or an actor, it behaves as if the
type `T` was used instead. That is, the semantics regarding assignments, function parameters, and
sending messages to other threads are as one would expect for `T`.

The type `Maybe<T>` has the following members:

- *copy constructor* and *assignment*

  Allows copying and assigning the `Maybe<T>`.

- *default constructor*

  Creates a `Maybe<T>` that contains `null`.

- *cast constructors*

  The type automatically generates typecast constructors that allows implicitly converting `T` and
  all subclasses of `T` into `Maybe<T>`. This is also true for all types `Maybe<U>`, where `U` is a
  subclass of `T`. Constructors are, however, not generated for types that are implicitly
  convertible between each other through custom conversions.

- `any()`

  Returns `true` if the `Maybe<T>` contains a `T`.

- `empty()`

  Returns `true` if the `Maybe<T>` is empty (i.e. contains `null`).

- `toS()`:

  Creates a string representation of the type. Returns either the string `null`, or the string
  representation of the contained object.


As noted above, the `Maybe<T>` type does not contain any means of accessing the contained element,
as this can not be done safely without special language constructs. Instead, languages (e.g. Basic
Storm) have to provide specialized, suitable syntax for this task. For example, Basic Storm allows
accessin elements in a `Maybe<T>` as follows:

```bs
void fn(Maybe<Int> x) {
    if (x) {
        print("X plus one: ${x + 1}");
    }
}
```

The syntax is described in further detail in the section on [weak
casts](md:/Language_Reference/Basic_Storm/Code/Type_Conversions) in the Basic Storm section of the
manual.
