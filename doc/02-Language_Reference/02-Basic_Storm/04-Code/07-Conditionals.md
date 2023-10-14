Conditionals
============

There are two conditional constructs in Basic Storm: the `if` statement and the `unless` statement.
As with most other constructs in Basic Storm, both of them are expressions, even if they most often
are used as statements.

This section focuses on the control structures themselves, and therefore only covers simple
conditions. A condition may also be a *weak cast*, which is described in the section on [type
casting](md:Type_Conversions).

If Statements
-------------

If statements follow the same structure as in C++ and Java:

```
if (<condition>)
    <true branch>
[else
    <false branch>]
```

The if statement evaluates `<condition>`, which is expected to evaluate to a `core:Bool`. If it is
`true`, then `<true branch>` is evaluated. Otherwise, `<false branch>` is evaluated if it exists.
The two branches can be arbitrary expressions, but in many cases they are blocks that contain the
code. Since `if` is a statement, it is possible to stack `if` statements as follows:

```
if (<condition 1>)
    <condition 1 is true>
else if (<condition 2>)
    <condition 2 is true>
else
    <otherwise>
```


The if statement evaluates to the value of whichever branch was taken. Since Basic Storm is
statically typed, this means that the if statement must be able to compute the type of the returned
value. This is done by examining the resulting type of the true- and false- branches in the
if-statement and computing a common type (e.g. common base classes). If this fails, or if the false
branch does not exist, then the if statement evaluates to `void` (i.e. nothing). Since the `else if`
construct is just nested if-statements, this works for arbitrarily nested if statements.

For example, this allows writing code like this:

```bs
Str greeting(Str name) {
    Str name = if (name.empty) {
        "anonymous";
    } else {
	name;
    } + "!";
    return "Hello " + name;
}
```

Note that a semicolon (`;`) is necessary after the closing bracket in the if statement. This is
required to let Basic Storm know when the expression ends. Furthermore, it is a good idea to use
blocks when using if statements in an expression context. Otherwise one would have to use duplicate
semicolons to delimit the end of the else statement and the entire expression.


Unless Statements
-----------------

A `unless` statement is mostly equivalent to a negated `if` statement. That is, the two functions
below are roughly equivalent.

```bs
void withIf(Bool condition) {
    if (!condition)
        return;
    // Do stuff...
}

void withUnless(Bool condition) {
    unless (condition)
        return;
    // Do stuff...
}
```

As can be seen from the examples above, the `unless` statement is designed to be useful when using
*early returns* or *guard clauses* to avoid deeply nested if statements. In many languages it would
be unnecessary to have a special construct for this concept. However, since Basic Storm supports
[weak casts](md:Type_Casting) that may fail, the language needs to be aware of what variables should
be accessible.

To illustrate this design, consider the function `getDefault` below. It takes a `Maybe<Int>` as a
parameter. Since `Maybe<Int>` represents a value that might be `null`, it is necessary to convert it
into a regular `Int` by checking it using a weak cast (explained in detail later). We can implement
the function in two ways, one with `if`, and the other with `unless`:

```bs
Int getDefault(Maybe<Int> check, Int default) {
    if (check) {
        // The weak cast above creates a new variable in this
        // scope: "Int check", that we can access.
        return check;
    }

    // Here, we can not access the "Int check" created
    // above, since the original value might have been null.
    return default;
}
```

```bs
Int getDefault(Maybe<Int> check, Int default) {
    unless (check) {
        // Here, the 'check' refers to the parameter
        // which has the type 'Maybe<Int>'.
        return default;
    }

    // Since the check in the 'unless' statement did not
    // fail, we now have a "Int check" variable that we
    // can access:
    return check;
}
```

The two versions are equivalent in this case. However, since it is not possible to negate a weak
cast with the `!` operator, we need to structure the code differently. This is the key reason for
the existence of the `unless` statement. As we see from this example, the `unless` statements
requires that the block after it either returns or throws an exception. Otherwise, Basic Storm can
not safely assume that `check` is non-null after the `unless` statement, and it would not be able to
safely provide a way to access the integer variable inside `check`.
