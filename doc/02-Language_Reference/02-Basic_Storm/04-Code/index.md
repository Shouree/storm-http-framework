Code
=====

This section of the manual covers the syntax and semantics of Basic Storm code. That is, anything
that may appear inside the body of a function.

Basic Storm is an imperative language. The body of a function is a block. Each block contains zero
or more local variables that are only accessible within that block. Each block also contains zero or
more statements that are delimited by semicolons (`;`). Blocks are statements (or actually,
expressions), so it is possible to introduce new blocks at arbitrary locations to limit the lifetime
of local variables.

One feature that sets Basic Storm aside from many other imperative languages is that all language
constructs are treated as expressions internally. The difference between a statement and an
expression is thus merely syntactical: statements are expressions that are separated by semicolons.
In some cases, such as for blocks, semicolons are optional to match the syntax of languages like C++
and Java. Since statements and expressions are the same, many language constructs may be used as a
part of an expression. Perhaps the most notable example of this is that blocks may be used as
expressions, and that they evaluate to the value of the last statement inside of them. This also
means that it is possible to return values from a function simply by making the last statement
evaluate to the desired return value.

For example, the following function returns the number 1:

```bs
Int returnOne() {
    1;
}
```

The remainder of this section will introduce the different expressions in Basic Storm in further
detail. In particular, blocks will be described in further detail [here](md:Blocks).
