Literals
========

This section lists all literals available in Basic Storm. Unless otherwise noted, literals generally
produce code that simply loads the literal into a machine register, and are cheap to use.

Booleans
--------

Booleans are denoted by the literal `true` or `false` in the source code. They evaluate to the type
`core:Bool` in the standard library.


Integers
--------

Integer literals are written using the digits `0` to `9` in base 10. Integer literals have the type
`core:Long` by default, but they may be implicitly casted to any integer type as long as the number
is small enough to fit the desired type.

Integer literals may be suffixed to explicitly determine the type of the variable. The letter of the
suffix is the first letter of the corresponding built-in type, as follows:

```bs
void literals() {
    Byte b = 10b;
    Int  i = 10i;
    Nat  n = 10n;
    Long l = 10l;
    Word w = 10w;
}
```

Hexadecimal literals are also supported. In contrast to integer literals, they are treated as
unsigned numbers, but they can of course be casted to signed numbers manually if desired. Similarly
to integer literals, hexadecimal literals may be suffixed to indicate their desired type. However,
to avoid ambiguity, an underscore is required before the suffix:

```bs
void literals() {
    Nat  any = 0x0129A;
    Byte b   = 0xFE_b;
    Nat  n   = 0xFEFF_n;
    Word w   = 0xFEFF_w;
}
```

Floating Point Numbers
----------------------

Floating point numbers are written in base 10, and are written using a dot (`.`) as the decimal
point. Scientific form is currently not supported. Floating point numbers are of the type
`core:Float` by default, but can be implicitly casted to `core:Double`. Similarly to integer
literals, floating point literals may be suffixed to indicate their type.

```bs
void literals() {
    Double a = 1;
    Double b = 1.2;
    Float  c = 1.2;
    Float  d = 1f;
    Float  e = 1.2f;
    Double f = 1d;
    Double g = 1.2d;
}
```


Strings
-------

String literals are enclosed in double quotes (`"`). A string literal has the type `core:Str`. Since
strings are immutable, Basic Storm creates an instance of a `Str` at compile time, and simply loads
the same instance of the string each time the string literal is evaluated.

Characters in strings may be escaped by a backslash (`\`). This allows having strings that contain
double quotes, or nonprintable characters. Supported escape characters are as follows:

- `\"` - a double quote
- `\n` - a newline
- `\r` - a carriage return
- `\t` - a tab
- `\v` - a vertical tab
- `\0` - the character 0


### Interpolated Strings

Basic Storm supports *interpolated strings* inside string literals. This makes it convenient to
format values as text. Variables are embedded as follows: `${<expr>}`, where `<expr>` is an
arbitrary Basic Storm expression. Because of this, it is typically necessary to escape the `$`
character in string literals as well.

When a string literal contains a `${}`, Basic Storm needs to create the string at runtime. As such,
using interpolated strings are associated with a run time cost. Interpolated strings are
automatically transformed to code that creates a string buffer (`core:StrBuf`), and appends the
interpolated values to it. As such, while it is not free to use interpolated strings, it Basic Storm
aims to make it as cheap as possible.

The syntax also supports formatting strings. Formatting can be specified by adding a comma followed
by formatting options. The supported options are as follows:

- *a number*: Indicates the minimum width, equivalent to `out << width(<number>) << <expr>`.
- `l`: Left-align this item. Equivalent to `out << left << <expr>`.
- `r`: Right-align this item. Equivalent to `out << right << <expr>`.
- `f<char>`: Set the fill character. Equivalent to `out << fill(<char>) << <expr>`.
- `x`: Output as hexadecimal. Note that this only works for unsigned numbers. Equivalent to `out << hex(<expr>)`.
- `d<number>`: Specify the number of digits for floating point numbers.
- `.<number>`: Specify the number of decimal digits for floating point numbers.
- `s<number>`: Specify the number of decimal digits in scientific notation for floating point numbers.

Formatting options are evaluated left to right. In case some options override others, the last one
will be visible.

As an example, it is possible to format strings into a table as follows:

```bs
void format(Str[] names) {
    for (k, v in names)
        print("${k,4}: ${v,10}");
}
```

Or to pad numbers with zeros like this:

```bs
Str leftPad(Int number) {
    "${number,f05}"; // returns 00123 when number = 123.
}
```

Additional options can be added by extending the rule `lang:bs:SIntFmt`.


Characters
----------

Character literals are written using single quotes (`'`). They need to contain exactly one codepoint. The
codepoint may be expressed using escape sequences, like the ones used in string literals. Character
literals evaluate to the type `core:Char`.


Arrays
------

Array literals are written in square brackets (`[]`), and with elements separated by commas. The
array literals evaluate to the type `core:Array<T>`, where the type `T` is inferred automatically
from the elements and the context when possible. If automatic inference is not possible, one may
specify the type of the array by prepending the type before the literal followed by a colon, like:
`Int:[1, 2, 3]`.

Since arrays are not immutable, evaluating an array literal creates a new copy of the array each
time.

For example, one may create an array as follows:

```bs
Int[] createArray(Int a, Int b) {
    return [a, b, 10];
}
```


Units
-----

Literals may be followed by a unit to convert it into an appropriate type. The units available are:

- `h` - generates a `Duration` with the specified number of hours.
- `min` - generates a `Duration` with the specified number of minutes.
- `s` - generates a `Duration` with the specified number of seconds.
- `ms` - generates a `Duration` with the specified number of milliseconds.
- `us` - generates a `Duration` with the specified number of microseconds.
- `deg` - generates an `Angle` with the specified number of degrees.
- `rad` - generates an `Angle` with the specified number of radians.

The literals are specified in the rule `lang:bs:SUnit`.

