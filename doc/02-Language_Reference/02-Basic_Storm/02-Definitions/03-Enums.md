Enums
=====

Basic Storm provides special syntax for enumeration types. An enumeration is a value type that
contains an integer (`core:Nat`) that is expected to contain one out of a number of pre-defined
values. An enum is defined as follows.

```
enum <name> [: bitmask] {
     <element> [= <value>],
     ...
}
```

This defines an enumeration type named `<name>`. If the `: bitmask` part is present, the
enumeration is a bitmask and may contain combinations of the values, rather than just a single
value. The body of the enumeration may contain a number of elements. Each row contains the name of
the element (`<element>`), and optionally the value of the element (`<value>`). For example, an
enumeration may look like this:

```bs
enum MyEnum {
    a = 0,
    b,
    c = 5
}
```

The values in the enum will then be accessible as `MyEnum:a`, `MyEnum:b`, etc. Similarly, a
bitmask is defined as follows:

```bs
enum MyBitmask : bitmask {
    a = 0x1,
    b = 0x2,
    c
}
```

The difference with a bitmask is that each value in the enum will be assigned a bit-pattern of
`0001`, `0010`, `0100` etc. to make it possible to represent the presence or absence of each
element. This may, of course, be overridden by the programmer if desired. Furthermore, a bitmask
defines operators to manipulate the bitmask, such as `+` for adding bits, `-` for removing bits,
and `&` (or `has`) for checking for overlap between two bitmasks.
