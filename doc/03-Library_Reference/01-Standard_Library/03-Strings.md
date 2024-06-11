Strings
=======

The standard library contains a type for strings, named [stormname:core.Str]. The types
[stormname:core.Object] and [stormname:core.TObject] both contain a member `toS` that creates a
string representation of objects. There is also a template function `toS` that converts values to
strings using the output operator for a string buffer. As such, most types can be converted to
string by typing `<obj>.toS()`.

The `Str` type is immutable, meaning that it is not possible to modify a string once it has been
created. It provides an interface where it is possible to access individual codepoints without
specifying an internal representation of the string. Since the internal representation is not the
same as the representation that is exposed, the `Str` class does not provide indexed access to the
codepoints (this would not be efficient). Rather, it is necessary to utilize
[iterators](md:Iterators) to refer to codepoints in the string. Since the iterators provide a `+`
operator, it is possible to step iterators a specific number of codepoints conveniently.

String Operations
-----------------

The `Str` class contains many functions for inspecting and modifying the string. Below is a
selection of its functionality, categorized by theme:

### Inspecting Content

```stormdoc
@core.Str
- .empty()
- .any()
- .count()
- .begin()
- .end()
- .==(*)
- .<(*)
```

Note that languages like Basic Storm automatically derives comparison operators from the ones that
are provided.

### Manipulation

```stormdoc
@core.Str
- .+(*)
- .escape()
- .unescape()
- .unescapeKeepBackslash(*)
- .toCrLf(*)
- .fromCrLf(*)
```

### Substrings

The following operations extract and inspect substrings:

```stormdoc
@core.Str
- .cut(*)
- .remove(*)
- .startsWith(*)
- .endsWith(*)
- .find(core.Char, core.Str.Iter)
- .find(core.Str, core.Str.Iter)
- .findLast(core.Char, core.Str.Iter)
- .findLast(core.Str, core.Str.Iter)
```

The second parameter to `find` and `findLast` is optional. If it is omitted, the search starts at
the start or end of the string respectively.

### Conversion

The `Str` class contains functions for converting strings to numbers. For this task it provides the
functions `toX` where `X` is one of the types `Int`, `Nat`, `Long`, `Word`, `Float`, or `Double`.
These functions all throw an exception if the format is invalid. There are also functions `isInt`,
`isNat`, and `isFloat` that checks if the string contains a signed integer, an unsigned integer, and
a floating-point number respectively. None of these functions verify the magnitude of the parsed
number.

Finally, the functions `isHex` can be used to check if the string contains a hexadecimal number, and
`hexToNat` and `hexToWord` can be used to convert hexadecimal numbers to unsigned integers.

### Other Utilities

There are also a few other utility functions provided:

```stormdoc
- core.removeIndentation(Str)
- core.trimBlankLines(Str)
- core.trimWhitespace(Str)
```

Characters
----------

The type `Char` represents a single unicode codepoint. This is the type that is returned from the
iterators in the `Str` type, and as such what `Str` essentially contains (however, `Str` uses a more
compact internal representation).


The String Buffer
-----------------

The string buffer, `StrBuf`, is a mutable string that is able to build strings efficiently. The
`toS` member function that exists for all types usually calls an overloaded version of `toS` that
accepts a [stormname:core.StrBuf] as a parameter. This makes it possible for objects to create their
string representation efficiently, rather than relying on string concatenation. For example, a `toS`
implementation for a simple class could look like below:

```bs
class MyClass {
    Int value;

    protected void toS(StrBuf to) : override {
        to << "My class: " << value;
    }
}
```

As can be seen above, the `StrBuf` class utilizes the `<<` operator to add strings to the end of the
string buffer. There is also a member `add` that can be used in languages where the `<<` operator is
not available (e.g. the Syntax Language). The string buffer contains overloads for the primitive
types in the standard library.

The string buffer also has the ability to format output. The following formatting options are
available:

```stormdoc
@core
- .width(Nat)
- .left()
- .left(Nat)
- .right()
- .right(Nat)
- .fill(Char)
- .precision(Nat)
- .significant(Nat)
- .fixed(Nat)
- .scientific(Nat)
- .hex(*)
```

The `StrBuf` also contains the members `indent` and `dedent` that automatically indents the output
by one additional of the indentation string. The indentation string can be set by calling
`indentBy`. There is also a class `Indent`, that can be used to indent a `StrBuf` as long as the
`Indent` object is in scope.
