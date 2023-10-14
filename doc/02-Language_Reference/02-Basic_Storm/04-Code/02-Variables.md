Variables
=========

Variables in Basic Storm are declared by writing `<type> <name>;`, similarly to C++ and Java.
Variables are always initialized. If nothing else is specified, the default constructor is called to
initialize the variable. To specify other initialization, one can write either `<type> <name> =
<expr>;` to initialize the value to the result of `<expr>`. It is also possible to call a
constructor by writing `<type> <name>(<params>);`. Local variables are accessible in the block they
are declared in, and they are automatically destroyed when the block is exited.

When a variable is initialized with the `=` syntax, Basic Storm can infer the type of the variable
if the type is specified as `var`. For example, the code below declares two variables of the type
`Str`:

```bs
void variables() {
    Str a = "hello";
    var b = "hello";
    Str c; // Empty string
}
```

Since variables are also expressions, they can be declared inside expressions. Variables still have
the scope of their enclosing block, and the declaration returns the assigned value of the variable.
Because of this, expressions like this are valid in Basic Storm: `1 + (Int a = 20);`. It is a good
idea to enclose things with parenthesis to make the intention clearer, both for you and the parser!
Expressions like: `1 + Int a = 20 + 3` are not clear what they mean. The usefulness of this kind of
expressions are found in if statements or loops, where it becomes possible to do like this:

```bs
void variables() {
    if ((Int i = fn(a)) < 3) {
        print(i);
    }
}
```

It also allows declaring variables in `for`-loops and `if`-statements without special syntax or
semantics.
