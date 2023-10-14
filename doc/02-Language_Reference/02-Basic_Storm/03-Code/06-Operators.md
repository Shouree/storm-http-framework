Operators
=========

Operators in Basic Storm simply call functions with the same name as the operator itself. For
example, the expression `1 + 2` is equivalent to `1.+(2)` (even though it is not possible to call
operators in that fashion). As such, overloading operators can be done simply by defining functions
with the appropriate names and parameter lists.

The syntax rule `lang:bs:SOperator` matches all operators in the language. The operators can be
divided into the following broad categories:

Binary Operators
----------------

These operators take two operands:

- `+` - addition
- `-` - subtraction
- `*` - multiplication
- `/` - division
- `%` - remainder
- `<<` - left shift
- `>>` - right shift
- `&` - bitwise AND
- `|` - bitwise OR
- `^` - bitwise XOR

Variants combined with assignments (e.g. `+=`) are also available for all operators. If a special
implementation is available, it is used. Otherwise, Basic Storm expands operators of the form `a +=
b` into `a = a + b` automatically.

Comparison Operators
--------------------

The following comparison operators exist:

- `<` - less than
- `>` - greater than
- `<=` - less than or equal
- `>=` - greater than or equal
- `==` - equal
- `!=` - not equal
- `is` - same object
- `!is` - not same object

These operators are similar to the binary operators above in that all but the `is` and `!is`
operators call overloaded comparison operators where available. The exception is that `==` and `!=`
operators compare object identity for actor types.

Comparison operators are also special in that the system attempts to implement missing operators in
terms of operators that exist. For example, if the operator `>` is not defined for some type,
expressions like `a > b` are automatically transformed into `a < b`. Similarly, if `>=` is not
present, the expression `a >= b` is transformed into `!(a < b)`. While this makes it possible to
implement all comparisons by only implementing the `<` operator, it is usually desirable to
implement at least the `==` operator as well, since `a == b` would otherwise have to be transformed
into `!(a < b) & !(b < a)`, which is likely not very efficient.


Prefix Unary Operators
----------------------

The following operators preceed a single expression:

- `!` - boolean negation
- `-` - integer negation
- `~` - bitwise NOT


Uniary Operators
----------------

The following unary operators have both postfix and prefix variants. Prefix operators call the
function named `++*` and postfix operators call the function named `*++`.

- `++` - increment
- `--` - decrement


String Concatenation
--------------------

The string concatenation operator `#` is special. It is used to concatenate strings efficiently
using a string buffer. As such, a sequence of `#` operators are transformed into a single operation
that creates a string buffer and concatenates all operands to it in order. The operator attempts to
convert each operand to a string before concatenation.


Assignment
----------

The assignment operator (`=`) is also a bit special. For value types, it calls the `=` member of the
value, but it provides a default implementation for class- and actor types that are manipulated by
reference.

Basic Storm also allows creating member functions that simulate a member variable. This is useful in
cases where get- and set functions are required but it is desirable to treat the concept as if it
was a regular member variable. This mechanism also helps a smooth transition from an implementation
using a member variable to an implementation that uses get- and set functions (e.g. to invalidate
caches or to notify other systems about a change to the variable).

Since Basic Storm does not differentiate between an empty list of actual parameters (i.e. `fn()`)
and no parameter list (i.e. `fn`), implementing the get function is trivial. Simply provide a member
function without any parameters, and the function can be used to read the value as if it was a
regular variable. The set functionality, however, requires an additional mechanism, called
*assignment functions* in Basic Storm. Assignment functions are regular function that are marked
using `assign` instead of a return type as follows:

```bs
class MyClass {
    init() {
        init() { data = 2; }
    }

    Int v() {
        data;
    }

    assign v(Int x) {
        data = x;
    }

    private Int data;
}
```

Declaring a function as `assign` makes the assignment operator consider using that function to
implement assignment to a particular member. In this case, the assignment operator in the expression
`c.v = 3` will realize that it is not possible to assign to the value `c.v` and look for an
assignment function `c.v(3)` to implement the assignment. Note that this transformation is only
performed if the function called by `c.v(3)` is marked as an assignment function, it is not done
in general.


```bs
void useClass() {
    MyClass c;
    c.v = 20; // equivalent to foo.v(20)
    print(foo.v.toS);
}
```

Array Access
------------

The array access operator (`[]`) is special in that it is not an inline operator, but rather it
works similarly like function calls. It is typically used to access elements in array-like types as
follows: `array[3]`. It is possible to overload it using the name `[]`.



