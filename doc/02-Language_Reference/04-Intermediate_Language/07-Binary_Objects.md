Binary Objects
==============

A [stormname:core.asm.Binary] object represents a [stormname:core.asm.Listing] that has been
transformed into machine code. The Binary itself contains the generated machine code and associated
metadata needed for exception handling.

A `Binary` object is created from a `Listing` that contains the code that should be compiled into
machine code, and an `core.asm.Arena` that describes which platform to compile for. As such, it *is*
possible to compile code for other platforms than the current one. Such code is, however, not
possible to execute. An `Arena` suitable for the current platform can be retrieved by calling the
function [stormname:full:core.asm.arena()].

Compiling a listing into machine code thus looks as follows in Basic Storm:

```bs
Binary compile(Listing l) {
    return Binary(core:asm:arena(), l);
}
```

As mentioned [previously](md:References), a `Binary` object inherits from the `Content` class, and
it is thus possible to specify the `Binary` as the content of a `RefSource` object, and thus call it
from other functions in the system.

To execute compiled code from Basic Storm, it is necessary to attach it to a `Function` object. The
example in the [next page](md:Example) illustrates how this is done.
