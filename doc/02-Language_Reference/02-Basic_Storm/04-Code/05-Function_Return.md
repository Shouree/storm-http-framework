Function Return
===============

As mentioned [previously](md:Blocks), a function automatically returns whatever value the topmost
block evaluates to. As such, it is possible to omit the return statement from a function:

```bs
Str makeGreeting(Str name) {
    StrBuf out; // Use a string buffer:
    out << "Hello ";
    out << name;
    out << "!";
    out.toS; // implicitly returned
}
```

This does, however, mean that instead of an error stating that a `return` statement is forgotten,
the error states that the types were incompatible. For example, if the last line in the
`makeGreeting` function above is omitted, a type error is returned since the last expression (`out
<< "!"`) evaluates to a `StrBuf` object.

It is also possible to use the `return` keyword to exit the execution of a function early. The
syntax resembles that of C++ and Java:

```bs
Str makeGreeting(Str name) {
    if (name.empty)
        return "Hello there!";

    StrBuf out; // Use a string buffer:
    out << "Hello ";
    out << name;
    out << "!";
    out.toS; // implicitly returned
}
```

For `void` functions, the expression after `return` can simply be omitted.