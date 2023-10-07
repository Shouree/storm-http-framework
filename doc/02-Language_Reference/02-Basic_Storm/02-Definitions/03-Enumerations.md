Enumerations
============

Basic Storm provides special syntax for enumeration types. An enumeration is a value type that
contains an integer (`core:Nat`) that is expected to contain one out of a number of pre-defined
values. An enum is defined as follows:

```
enum <name> [: bitmask] {
     <element> [= <value>],
     ...
}
```

This defines an enumeration type named `<name>`, with zero or more `<element>`s that specify
possible value. Each element will be assigned a value automatically, but it is possible to specify a
value manually if desired.

If the `: bitmask` option is present, the enumeration is a bitmask. This means that instead of
representing one out of the possible elements, it may contain a combination of zero or more of the
elements.


Normal Enumerations
===================

For non-bitmask enumerations, the value for elements are assigned from zero and upwards. Consider
the following example:

```bs
enum MyEnum {
    a, // will be 0
    b, // will be 1
    c = 10, // will be 10
    d, // will be 11
}
```

Each element is represented by a static member function in the enumeration that returns an instance
with the appropriate value. The type does not have a default constructor, so the generated functions
for the element are the preferred way to create instances of the type. For example, the element `a`
can be accessed with the name `MyEnum:a`.

In addition to the elements, the type contains the following members (apart from copy- and
assignment):

- Initialization constructor: Creates an instance of the enum with an arbitrary value (e.g. `MyEnum(22)`).
- `toS`: Creates a string representation of the enumeration based on the elements.
- `v`: Get the value from the enumeration.
- `==` and `!=`: Compare the enumeration to another value.


Bitmask Enumerations
====================

The assignment of values to elements work differently for bitmasks. The first element is assigned
the value `0x1`, and subsequent values are assigned a value with the next higher bit set. For
example:

```bs
enum MyBitmask : bitmask {
    a, // will be 0x1
    b, // will be 0x2
    c = 0x10, // will be 0x10
    d, // will be 0x20
}
```

It is possible to set elements to values that have more than one bit set, or values that have zero
bits set. These work as expected. The `toS` function will correctly handle values with zero bits
set, and greedily finds elements that correspond to the value to provide a string representation.
The generated string representation may, however, not always be minimal when elements with multiple
bits set exist.

A bitmask enumeration contains the following members in addition to those in a normal enumeration:

- `+`: Combine two enumerations, creating the union of the bits inside (compare to bitwise or).
- `-`: Remove set bits from the right hand side (compare to and with negation).
- `&`: Check if two bitmasks have intersecting bits.
- `has`: Same as `&`.
