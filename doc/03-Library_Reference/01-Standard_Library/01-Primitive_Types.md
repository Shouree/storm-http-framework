Primitive Types
===============

One thing that might be surprising about Storm is that primitive types are not treated specially by
the system. Rather, they are implemented as any other type in the standard library. As far as Storm
is concerned, the primitive types in the standard library are just opaque types with a set of
operations that manipulate the opaque representation. That is, these types have no members, and
therefore need to be manipulated through the members in the type. The only thing that is special
about the primitive types is that they are treated specially during function calls (to follow the
system's calling convention, to allow seamless calls to and from C++).


The standard library provides the following primitive types:

- `core.Bool`

  A type that may contain one of the values `true` and `false`.

- `core.Byte`

  An 8-bit unsigned integer.

- `core.Int`

  A 32-bit signed integer.

- `core.Nat`

  A 32-bit unsigned integer. `Nat` is short for "natural number".

- `core.Long`

  A 64-bit signed integer.

- `core.Word`

  A 64-bit unsigned integer.

- `core.Float`

  A 32-bit floating point number.

- `core.Double`

  A 64-bit floating point number.


All types have members that implement the expected comparison and arithmetic operators. Types are
converted between each other using members in the types. For example, the numeric types contain a
member `int` that converts them to an integer.


The system also provides the following convenience functions:

- `min`, `max` - minimum and maximum values
- `sqrt` - square root
- `pow` - exponential
- `abs` - absolute value
- `clamp` - limit in a range
