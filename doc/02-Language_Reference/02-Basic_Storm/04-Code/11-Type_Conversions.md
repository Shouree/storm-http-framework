Type Conversions
================

Basic Storm attempts to limit automatic typecasts to avoid situations where information is lost due
to an unintended implicit type conversion. This is, however, overly strict in many situations, so
Basic Storm allows some level of implicit type conversions that are considered safe.

The remainder of this section describes the different type conversions available, both explicit and
implicit ones.


Explicit Type Conversions
-------------------------

There is technically no standard way to perform explicit type conversions in Basic Storm. Instead,
Basic Storm relies on the standard library. All primitive types contain member functions for
conversion between the relevant member types. For example, one can convert from an integer
(`core:Int`) to an unsigned integer (`core:Nat`) by calling the `.nat` member available in the `Int`
type (and most other integer types). Some examples are as follows:

```bsstmt
10i.nat;   // Convert Int => Nat
10n.int;   // Convert Nat => Int
10.0.int;  // Convert Float => Int
```

Most types also have a `toS` (short for `toString`) that converts it to a string. Value types may
instead have an output operator `<<`. If that is the case, then a `toS` function is automatically
generated for them in the system.

Other user-defined types typically have constructors for converting between types. As such, for two
types might be possible to explicitly call the constructor to convert between the types. For
example:

```bsstmt
T original;
U copy(original); // Constructor call at creation time
U(original);      // Constructor call as a part of an expression.
```

Implicit Type Conversions
-------------------------

As previously mentioned, some type conversions are performed implicitly. Basic Storm allows the
following conversions implicitly:

- Converting from a derived type to a super type.

- Calling a constructor of the target type that is marked as a *cast constructor* (keyword `cast` in
  Basic Storm). Note that this is only checked in one level. Namely, if an implicit conversion from
  `T` to `U` is required, Basic Storm will only attempt to check if `U(t)` is possible. As such, the
  implicit conversion will not be allowed even if there exists a type `V` that could be used as
  `U(V(t))`.

- Converting from `T` to `Maybe<T>` as required. This is actually implemented by `Maybe<T>` having
  cast constructors. However, there are some special cases that Basic Storm takes explicit care of
  in this regard (namely in if-statements).

- [Integer literals](md:Literals) can be automatically converted to another type, assuming the value
  of the literal is not too large for the target type. For example, the literal `0xFF` can be
  automatically casted from `core:Word` to `core:Byte` and `core:Nat` since it is small enough.
  However, `0x100000000` is too large to fit in both `core:Byte` and `core:Nat`, so those
  conversions are not allowed.

- Any other conversions implemented by custom AST nodes. For example, the array literals and the
  function pointers utilize this mechanism to automatically infer types in some cases. The scope of
  these are, however, generally limited and not surprising.


Weak Casts
----------

Certain type conversions do not always succeed, but may fail at times. Examples of such casts are
casts from a super type to a derived type, and casts from `Maybe<T>` into `T`. The cast from a super
type to a derived type only succeeds if the dynamic type of the variable matches the expected type,
and the cast from `Maybe<T>` to `T` only succeeds if the `Maybe<T>` did not contain `null`.

To accommodate this type of casts in a safe way, Basic Storm introduces the concept of a *weak
cast*. These casts need to be embedded in a conditional statement so that the compiler can ensure
that it is only possible to access the result of the cast if it succeeded.

The remainder of the documentation on weak casts will use `if` statements to illustrate the
behavior. However, it is possible to use weak casts in `unless` statements and `while` loops as
well.

A weak cast has the following form in general:

```bsstmt:placeholders
if (<variable> = <weak cast>) {
    // <variable> is visible here
} else {
    // <variable> is not visible here
}
// <variable> is not visible here either
```

The `<weak cast>` itself can take different forms depending on the cast that is attempted.
Currently, the following two forms are implemented. More can be added by extending the rule
`lang:bs:SWeakCast`:

- `<exression>`: Cast from a `Maybe<T>` into `T`.

- `<expression> as T`: Cast from a super type to a derived type, or from a `Variant` to the
  specified type. Also handles cases where `<expression>` is a `Maybe`-type.

For example, they can be used as follows:

```bs
void myFunction(Base? x) {
    if (d = x as Derived) {
        // 'd' is now of type Derived.
    }
}
```

As a special case, whenever `<expression>` only refers to a local- or member variable, the
`<variable>` part in the `if` statement may be omitted. In that case, Basic Storm creates a local
variable that shadows the original variable instead. As such, the following two are equivalent:

```bs
void myFnA(Base? x) {
    if (x = x as Derived) {
        // 'x' is now of type Dervied.
    }
}

void myFnB(Base? x) {
    if (x as Derived) {
        // 'x' is now of type Dervied.
    }
}
```

Of course, this works with casts from `Maybe<T>` into `T` as well. This makes null-checks look like
in C++:

```bs
void myFn(Base? x) {
    if (x) {
        // 'x' is now of type Base (i.e. not Maybe<Base> as the parameter is).
    }
}
```

The `<variable>` part may also be omitted for other expressions. However, in that case there is no
way to access the result of the successful cast.


For completeness, we provide the following examples of using weak casts with `unless` and `while`
statements:

```bs
void f(Int? to) {
    unless (to)
        return -1;

    to + 20;
}
```

Note that the branch following the `unless` statement needs to return or throw an exception to
ensure type-safety of the `to + 20` part of the code. The code above is equivalent to the following:

```bs
void foo(Int? to) {
    if (to) {
        to + 20;
    } else {
        -1;
    }
}
```

Finally, the ability to use weak casts in `while` loops makes it convenient to use the iterators
from `WeakSet` among others:

```bs
void iterate(WeakSet<Int> set) {
    var iter = set.iter();
    while (elem = iter.next()) {
        // Do something
    }
}
```
