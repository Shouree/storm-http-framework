Strings
=======

The standard library contains a type for strings, named `Str`. The classes `Object` and `TObject`
both contain a member `toS` that creates a string representation of objects. There is also a
template function `toS` that converts values to strings using the output operator for a string
buffer. As such, most types can be converted to string by typing `<obj>.toS()`.

The `Str` type is immutable, meaning that it is not possible to modify a string once it has been
created. It provides an interface where it is possible to access individual codepoints without
specifying an internal representation of the string. Since the internal representation is not the
same as the representation that is exposed, the `Str` class does not provide indexed access to the
codepoints (this would not be efficient). Rather, it is necessary to utilize
[iterators](md:Iterators) to refer to codepoints in the string.

The string class contains many common functions for string modification. For example:

- `+` - concatenate strings
- `==`, `!=` - comparison
- `cut` - create a substring
- `find` - search for characters or substrings
- `findLast` - reverse search
- `isInt`, `isNat`, `isFloat` - check if the string is a number (ignoring the range)
- `toInt`, `toNat`, `toLong`, `toWord`, `toFloat`, `toDouble` - convert the string to other types
- `escape`, `unescape` - escape and unescape escape sequences in the string


Characters
----------

The type `Char` represents a single unicode codepoint. This is the type that is returned from the
iterators in the `Str` type, and as such what `Str` essentially contains (however, `Str` uses a more
compact internal representation).


The String Buffer
-----------------

The string buffer, `StrBuf`, is a mutable string that is able to build strings efficiently. The
`toS` function in `Object` and `TObject` actually calls an overloaded `toS` function that accepts a
`StrBuf` as a parameter, so that objects can construct their string representation efficiently. For
example, it is possible to implement a `toS` method as follows:

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

- `width` - set the width of the next outputted item
- `left` - left align the next output, optionally with a specified width
- `right` - right align the next output, optionally with a specified width
- `fill` - set a character to use for padding to the specified width
- `precisison` - set precision for floating point output
- `significant` - set output to a maximum number number of decimal places for floating point numbers
- `fixed` - set output of floating point numbers to have a fixed number of decimal places
- `scientific` - set output to scientific notation with the specified number of significant digits
- `hex` - output the specified number in hexadecimal

The `StrBuf` also contains the members `indent` and `dedent` that automatically indents the output
by one additional of the indentation string. The indentation string can be set by calling
`indentBy`. There is also a class `Indent`, that can be used to indent a `StrBuf` as long as the
`Indent` object is in scope.
