Blocks
======

As mentioned in the introduction, blocks are expressions in Basic Storm. Blocks evaluate to the
value of the last expression inside of them (similarly to `progn` in Common Lisp, and how blocks
work in Elixir). Since the body of a function is a block, it is possible to omit the return
statement as long as the last statement in the function body evaluates to a value of the correct
type.

Blocks behave in this way to make it easy to create syntax extensions that need to introduce extra
temporary variables. Since blocks behave as an expression, it is possible to introduce such extra
variables almost anywhere. This is used by the interpolated string literals to create code like
this:

```bs
Str before() {
    Str world = "World";
    "Hello" + ", ${world}";
}

Str expanded() {
    Str world = "World";
    "Hello" + {
        StrBuf tmp;
        tmp.add(", ");
        tmp.add(world);
        tmp;
    };
}
```
