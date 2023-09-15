Expressions
============

Expressions in Basic Storm are anything you can write inside a function. Separate expressions are,
like in C, separated by a semicolon (`;`). Note that there are currently no real statements in Basic
Storm. What sets Basic Storm apart from C is that everything can appear as a part of an expression,
including local variable declarations, blocks and if-statements.

Syntax
-------

The syntax used to describe expressions have their roots in the `Expr` syntax rule. There is also a
rule named `Stmt`. The difference between an expression and a statement on the syntax level is that
a statement ends with a semicolon, while an expression does not. This distinction is done to allow
blocks to not end in a semicolon, as they do in C. It is, however, possible to include blocks in a
regular expression as well. After the syntax level, expressions and statements are treated equally,
as expressions.


Literals
---------

Basic Storm supports the following literals:
* __Strings:__ enclosed in double quotes (`"abc"`). A string evaluates to a `core:Str` 
  object. The `Str` objects corresponding to the string literals are generally stored
  alongside the code, so that no new objects are created when using a string literal.
  However, this is not the case when using interpolated strings (see below).
* __Integers:__ a simple number. Integer literals evaluate to a `core:Int` by default,
  but the compiler will cast the literal to `core:Nat` or `core:Byte` automatically if the context 
  requires it, and the literal fits inside the target type without truncation. In some cases, it 
  is necessary to manually help the compiler by doing the casts manually. This is done by calling
  the `nat` or `byte` method on the `Int` object, like this: `1.nat`, or `nat(1)`.
* __Reals:__ real numbers. Evaluate to the type `core:Float` or `core:Double`. Integer numbers are
  automatically casted to `core:Float` or `core:Double` if possible without a loss of precision.
* __Booleans:__ the reserved words `true` or `false`. Evaluates to `core:Bool`.
* __Arrays:__ enclosed in square brackets (`[]`), separated with comma (`,`). The literal may start with
  the desired type of the array followed by a colon (`:`). When the type is not entered, Basic Storm tries
  to infer the type according to the types in the array, and according to context. Arrays evaluate to an
  instance of the type `core:Array<T>`. Example: `Int:[1, 2, 3]` or `[1, 2, 3]`. Array literals are 
  implemented completely in Basic Storm, have a look at `lang:bs:array.bs`.

Numeric literals may be suffixed with a lowercase letter to indicate the desired type of the
literal. If a type is indicated in this manner, the literal will not perform automatic type casting
(however, the numeric types allow automatic casting to a type with higher precision using auto-cast
constructors, this is not disabled). The letter used is the first letter of the corresponding built
in type:

```
Byte   b = 10b;
Int    i = 10i;
Nat    n = 10n;
Long   l = 10l;
Word   w = 10w;
Float  f = 10f;
Double d = 10d;
```

Hexadecimal numbers are always treated as unsigned numbers (but they may be explicitly casted to
signed numbers at will). Hexadecimal numbers may also be suffixed as regular numbers. However, the
literals and the number must be separated by an underscore, like so (note that only unsigned types
are usable):

```
Byte   b = 0xFE_b;
Nat    n = 0xFEFF_n;
Word   w = 0xFEFF_w;
```

Interpolated strings
---------------------

Storm supports *interpolated strings* inside string literals. This means that it is possible to
conveniently concatenate strings like this: `"Hello ${name}"`, where `name` is an expression that
evaluates to a string. Because of this, it is necessary to escape the `$` character in addition
to `"`, and `\`. Using this syntax is equivalent to using the string concatenation operator
(i.e. `#`, see below), which is in turn equivalent to creating a `StrBuf` to do the concatenation
(use the `dump{}` statement in `lang:bs:macro` to inspect the behaviour if you are interested). This
means that even though it may not look like it, Basic Storm will type-check the interpolations for
you, and will inform you if you are doing something ill-formed. Since a `StrBuf` is used for
concatenation, this syntax does not have any additional overhead compared to using a `StrBuf`
directly. However, this means that string literals using interpolation has to be re-created each
time they are evaluated, in contrast to plain string literals. This can be observed using the `is`
operator.

This syntax also supports formatting of strings. To specify formatting, add a comma followed by
formatting options. The following formatting options are supported:

* *a number*: Indicates the minimum width, equivalent to `out << width(<number>)`.
* __l__: Left-align this item. Equivalent to `out << left`.
* __r__: Right-align this item (the default). Equivalent to `out << right`.
* __f<char>__: Set the fill character. Equivalent to `out << fill(<char>)`.
* __x__: Output as hexadecimal. Note that this only works for unsigned numbers. Equivalent to
  `out << hex(<expression>)`.
* __d__ (followed by a number): Output the floating point number with at most the specified number of digits.
* __.__ (followed by a number): Output the floating point number with the specific number of decimal digits.
* __s__ (followed by a number): Output the floating point number in scientific notation with the specified number of decimal digits.

The formatting is evaluated left to right, so in case any formatting overrides some other
formatting, only the last one will be visible.

For example, one can format strings into a nice table like this:

```
Str[] names;
for (k, v in names)
   print("${k,4}: ${v,10}");
```

Or pad numbers with zeros like this:

```
Int number = 123;
print("${number,f05}"); // prints 00123
```

Note that any Basic Storm expression is usable inside string interpolation. It is possible to do
things like this, even though it is not recommended: `"8 + 20 = ${8 + 20}"` or `"${a.toS + "b",20}"`.

Function calls
---------------

Function calls looks much like function calls in most other languages, with some differences. The
easiest way to explain these differences is to explain how Basic Storms treats function
calls. Whenever Basic Storm sees an identifier (`foo`, or `foo(a, b)`), it constructs a `Name`
object that represents the seen identifier (resolving the type of any parameters in the process) and
asks the type system to look it up (see [Names](md://Storm/Names.md)). This means since `foo`
and `foo()` are represented by the same `Name`, they are treated equally. Basic Storm then examines
the `Named` returned by the lookup and acts accordingly. If it is a function, it calls the
function. If it is a variable it loads the value of the variable.

The interesting parts does not end here. Basic Storm also implements a custom lookup that not only
looks in the global namespace. It also looks for members in the type of the first parameter of the
function call. This means that `a.foo()` and `foo(a)` are treated almost the same way. There is a
slight difference between the two in the context of a member function. In a member function `foo(a)`
means that the `this` pointer may be used to form `this.foo(a)` if it makes sense, where `a.foo()`
does not allow for that.

This is mainly done for flexibility and extensibility. With this scheme it is possible to add
members to classes from other places by adding something that looks like a member function without
having to modify the class itself. A good example of this from C++ is operator overloading and
IO-streams. The `<<` operator can be overloaded from anywhere to provide new functionality to the
output stream. Storm takes this idea and uses it for all functions.

The indifference between `foo` and `foo()` is useful when implementing something that should look
like a member variable, but can not be a member variable for some reason. A good example at the
moment is to implement read-only member variables. Since there is no support for constant member
variables, a getter function can be used to emulate this behavior. This is done for
`core:Array:count`. Later, Basic Storm will also introduce setter methods that will be executed
whenever someone tries to assign to a member variable that does not exist. This is not done yet.

Operators
----------

Operators in storm are defined by the `Operator` syntax rule. Operators are re-ordered after they
are parsed, to make it easier to understand the syntax rules, and thereby make it easier to add new
operators. The `Operator` rule is expected to return an `OpInfo` object which contains information
about the operator's priority and associativity. This object is also asked about what the operator
means.

By default, an operator is equivalent with calling the member function with the same name as the
operator on the left hand side object. This means that `1 + 2` is equivalent to `1.+(2)`. Note that
it is not possible to write function calls like this at the moment because of restrictions in the
syntax. However, it is possible to declare functions named after an operator. Operators with a pre-
and post variant uses a star to differentiate the semantics. The pre increment operator is named
`++*` while the post increment operator is named `*++`.

The `is` operator is an exception to this rule as it can not be overloaded. The `is` operator checks
whether two references refer to the same object. This is equivalent to the behaviour of `==` for
actor types, but the `is` operator works for class types as well, where `==` can be overloaded.

New operators can be implemented by adding a new option to the `Operator` rule, either using the
default `lOperator` or `rOperator` to follow the same semantics as most of the built-in operators,
or create a custom object overriding `OpInfo` and implements a custom meaning for the operator.

The comparison operators `==`, `!=`, `<`, `>`, `<=` and `>=` have slightly special semantics so that
it is not necessary to define all operators for all types. If one of these operators are not present
for the types used, Basic Storm tries to fall back on simpler operators. For example, the expression
`a > b` falls back to `a < b` if the first is not present, and `a >= b` falls back to `!(a < b)`.
This means that it is sufficient to implement `<` for a class to be able to do all comparisons. However,
it is usually desirable to also implement `==`, since that can usually be made more efficient than calling
`<` twice.

Currently, two operators have a special meaning: assignment and string concatenation. The assignment
operator will simply call `a.=(b)` if `a` and `b` are values, but it provides a default
implementation of the assignment operator for class and actor types. As described below, it is also
possible to create member functions that can be assigned to just as if they were a member
variable. This is considered by the default implementation of the assignment operator. The string
concatenation operator, `#`, is implemented in Basic Storm in `lang:bs:concat.bs` and uses a
`core:StrBuf` class to build a string from all elements involved. The same string buffer is used for
all elements, even if there are more than two present. Elements are appended to the `StrBuf` object
using the `add` member of `StrBuf`. `StrBuf` contains overloads for strings, integers and
booleans. If an `add` overload is not found, the value is converted to a string using the `toS`
function for the object (either member or non-member, just like function calls).

To implement operators like `+=`, Basic Storm takes a slightly different approach compared to
C++. This kind of operators (hereby called _combined operators_), are implemented by a syntax rule
that is applied before the regular `Operator` rule. This rule is named `SOperator`, and only has two
options, one which passes the `Operator` rule through it, and one that matches an `Operator` with an
`=` on the end. When the second option is matched, an instance of the `CombinedOperator` class is
created for the combined operator. This will first attempt to call the member function, just like
the default operators would do, for example `a += 1` tries to call `a.+=(1)`. If this is not
possible for some reason, it falls back to rewriting the expression from `<lhs> <op>= <rhs>` to
`<lhs> = <lhs> <op> <rhs>`. This means that if `a.+=(1)` is not callable in the example above, 
`a = a + 1` will be tried. This means that you get all the combined operators for free in Basic
Storm, but you can still provide more efficient implementations if you want. In C++, you always have
to implement these operators, even though the meaning is seldom ambiguous.


Assignment
-----------

Basic Storm allows creating member functions that simulate a member variable. This is useful in
cases where get- and set functions are required but it is desirable to treat the concept as if it
was a regular member variable. This mechanism also helps a smooth transition from an implementation
using a member variable to an implementation that uses get- and set functions (e.g. to invalidate
caches or to notify other systems about a change to the variable).

Since Basic Storm does not differentiate between an empty list of actual parameters (i.e. `foo()`)
and no parameter list (i.e. `foo`), implementing the get function is trivial. Simply provide a member
function without any parameters, and the function can be used to read the value as if it was a
regular variable. The set functionality, however, requires an additional mechanism, called
*assignment functions* in Basic Storm. Assignment functions are regular function that are marked
using `assign` instead of a return type as follows:

```
class Foo {
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
`foo.v = 3` will realize that it is not possible to assign to the value `foo.v` and look for an
assignment function `foo.v(3)` to implement the assignment. Note that this transformation is only
performed if the function called by `foo.v(3)` is marked as an assignment function, it is not done
in general.


```
Foo foo;
foo.v = 20; // equivalent to foo.v(20)
print(foo.v.toS);
```


Variables
----------

Variables are declared like in C, by writing `<type> <name>`. Variables can be initialized at
creation by either `<type> <name> = <expr>`, or <type> <name>(<params>)`. When a variable is
initialized using the assignment syntax, the type can be inferred by using the syntax 
`var <name> = <expr>`. Any local variables live as long as the scope they are declared in.

Since variables are also expressions, they can be declared inside expressions. Variables still have
the scope of their enclosing block, and the declaration returns the assigned value of the variable.
Because of this, expressions like this are valid in Basic Storm: `1 + (Int a = 20);`. It is a good
idea to enclose things with parenthesis to make the intention clearer, both for you and the parser!
Expressions like: `1 + Int a = 20 + 3` are not clear what they mean. The usefulness of this kind of
expressions are found in if statements or loops, where it becomes possible to do like this:

```
if ((Int i = fn(a)) < 3) {
   print(i);
}
```

It also allows declaring variables in for loops without anything special.

Blocks
--------

Blocks are also expressions in Basic Storm. Blocks return the value of the last expression inside
them, much like `progn` in Lisp. Functions work the same way, the function returns the result of the
outermost block, which is the value of the last statement in the block. This means that it is
possible to introduce a new block with temporary variables inside an expression, which is very
useful when implementing new syntax. Compare this to macros in C, where you have to express your
entire macro as an expression if you want it to be able to return a value. This makes it difficult
(if not impossible) to introduce temporary variables in C-macros. In Basic Storm it is easy, just
introduce a new block:

```
Str result = "Hello" + {
    StrBuf tmp;
    tmp.add(", ");
    tmp.add("World");
    tmp;
} + "!";
```

This is exactly how the string concatenation operator is implemented. The code above will be
generated by `Str result = "Hello" + (", " # "World") + "!";`. As mentioned before, this would be
hard, if not impossible, to do using regular C-style macros.

Return
-------

There are two ways of returning values in Basic Storm. By default the last expression in a block
will be considered as that block's return value, just like `progn` in lisp. The second version is to
use an explicit `return` statement. When a `return` statement is used inside a block, that block
will not be considered to return a value to any enclosing expression. This means that expressions
like this will still work as expected:

```
Str result = if (x == 3) {
    return 12;
} else {
    "hello";
}
```

Here, the compiler can see that the non-string result returned from the block inside the if-branch
inside the if-statement never returns normally, and therefore considers the if-statement to always
return a string.

Of course, return can also be used to exit functions returning `void`.

Async
------

When Basic Storm calls a function that should run on another thread, it will by default send the
message and wait until the called function has returned. During this time the current thread will
accept new function calls from other threads. If you wish to not wait for a result, use the `spawn`
keyword right before the function call. This makes the function call return a `Futre<T>` instead,
and you can choose when and if you want to get the result back.

Automatic type casting
----------------------

Basic Storm has support for automatic type casting, in a way that is similar to C++. However, in
Basic Storm, less conversions are implicit. Automatic conversions only upcast types (i.e. to a more
general type), or if a constructor is declared as `cast ctor(T from)`. For the built in types, very
few constructors are declared like this, so Basic Storm does not generally do anything unexpected.

The major usage of automatic casts in the standard library is the `Maybe` type, this is used to
allow `Maybe`-types to behave like regular object references.

In the `if` statement, Basic Storm tries to find a type that can store both the result from the true
and the false branch. At the moment, this is not too sophisticated. A common base class is chosen if
present (this works for `Maybe` as well). Otherwise, Basic Storm tries to cast one type to the
other, and chooses the combination that is possible. If all of these fails, Basic Storm considers it
is not possible to find a common type, even if there is an unrelated type that both the true and
false branch can be casted to. In this case, it is necessary to explicitly help the compiler to
figure out the type.

The automatic casting of literals is limited to very simple cases at the moment, and sometimes it is
necessary to explicitly cast literals (for example in `if` statements).

Weak casts
-----------

Aside from casts that always succeed, there are casts that might fail. These include downcasts, and
casts from `Maybe<T>` to `T`. In basic storm, these have a special syntax coupled with the
if-statement, to make sure that the casts are always checked for failure. Weak casts look like this:

```
if (<weak cast>) {
    // The weak cast succeeded.
}
```

or

```
if (x = <weak cast>) {
    // The weak cast succeeded.
}
```

If the variable name is omitted, the variable that was mentioned inside the weak cast is made to the
type of the cast in the if-branch. For example: to cast the variable `o` to a more specific type,
one can write like this:

```
if (o as Foo) {
    // o has the type Foo inside of here.
}
```

Which is equivalent to:

```
if (o = o as Foo) {
    // Success.
}
```

Casting from `Maybe<T>` to `T` is done just like in C++:

```
if (x) {
    // x is T here, not T?
}
```

or

```
if (y = x.y) {
    // y is T here, not T?
}
```

It is possible to combine these two weak casts. If you wish to cast a variable from `Maybe<T>` to a
more specialized type of `T` at the same time, the `as` weak cast is enough.

There is also an inverse form of weak casts, since in C++, it is sometimes common to check for
something and then return early if that condition does not hold. This is something that is not
possible using the regular weak casts. For example, in C++ one can do:

```
int foo(int *to) {
    if (!to)
        return -1;

    return *to + 20;
}
```

But in Basic Storm, one has to do:

```
void foo(Int? to) {
    if (to) {
        to + 20;
    } else {
        -1;
    }
}
```

which gets cumbersome if there are a lot of this kind of checks. Therefore, Basic Storm also
provides the `unless` block for this kind of tests. `unless` only works with weak casts.

```
void foo(Int? to) {
    unless (to)
        return -1;

    to + 20;
}
```

This is, as we can see, more similar to the C++ version, but keeps the null-safety of Basic
Storm. Aside from that, it is more readable if there are multiple checks for null and/or downcasts.

Finally, since some iterators (for example, `WeakSet`) has a single member `next` that returns a
`Maybe<T>` to indicate either the next element or the end of the sequence. In this situation it is
useful to also use weak casts in loop conditionals as follows:

```
void foo(WeakSet<Foo> set) {
    var iter = set.iter();
    while (elem = iter.next()) {
        // Do something
    }
}
```

As can be seen from the above example, weak casts in loops work just like for if-statements.


Function pointers
------------------

Function pointers are represented by the type `core:Fn<A, B, ...>`, where the first parameter
to the template is the return type, and the rest are parameter types. To create a pointer to a
function, use this syntax:

```
Fn<Int, Int> ptr = &myFunction(Int);
```

In this case, we assume that `myFunction` returns an `Int`. As you can see, the parameters of the
function has to be declared explicitly. This may not be necessary later on.

Function pointers may also contain a this-pointer of an object or an actor (not a value). This is
done like this:

```
Fn<Int, Int> ptr = &myObject.myFunction(Int);
```

As you can see, the type of a function pointer bound with an associated object is identical to that
of a function pointer without an associated object. This means that you can easily create pointers
to functions associated with some kind of state and treat them just like regular functions.

Function pointers are also as flexible as the regular function calls. This means that if both:

```
&object.function(A);
```

and

```
&function(Object, A);
```

works. In the first case, the function pointer will only take one parameter, while it will take two
in the second case. Bu utilizing this, it is possible to choose if the first parameter of a function
should be bound or not.

In cases where Basic Storm can easily infer the desired type of a function pointer (such as when
initializing a variable or when passing it as a parameter to a function), the parameter types may be
left out entirely:

```
Bool compare(Int a, Int b) {
    a > b;
}

void mySort(Int[] x) {
    x.sort(&compare);
}
```

Note the difference between omitting the parentheses entirely and supplying an empty parameter
list. The former means to automatically infer the function to be called, while the second means to
find a function taking zero parameters.

To call the function from the function pointer, use the `call` member of the `Fn` object.

Note that the threading semantics is a little different when using function pointers compared to
regular function calls. Since the function pointer does know where it is invoked from (and to make
interaction with C++ easier), the function pointer decides runtime if it should send a message or
not. This means that in some cases, the function pointer sees that a message is not needed when a
regular call would have sent a message (for example, calling a function associated to a thread from
a function that can run on any thread). The rule for this is simple, whenever the function pointed
to is to be executed on a different thread than the currently executed one, we send a message. At
the moment, it is not possible to do `spawn` when calling functions using function pointers.

The function pointer type can also be declared as follows:

```
fn(Int, Int)->Int => Fn<Int, Int, Int>
```

Different parts of can be left out. The following lines are equivalent:

```
fn()->void
fn->void
fn()->
fn->
```

This syntax is implemented in `lang:bs:fnPtr.bs`.


Lambda functions
-----------------

Lambda functions are anonymous functions that are easily created inline with other code in Basic
Storm. A lambda expression in Basic Storm evaluates to a function pointer pointing to the anonymous
function. A lambda function is created as follows:

```
var x = (Int a, Int b) => a + b + 3;
```

This creates a function and binds it to `x`, which will be of the type `fn(Int, Int)->Int`, or
`Fn<Int, Int, Int>`. In some cases, Basic Storm can infer the types of the parameters from
context. In such cases, they may be left out. This can be done when the lambda function is passed
directly as a parameter to a function, or if it is being assigned to a variable. For example:

```
Array<Int> x; // initialize with some data.
x.sort((a, b) => a > b);
```

In this case, Basic Storm is able to see that `sort` requires a function taking two `Int`s, and is
therefore able to infer that the types of `a` and `b` need to be `Int`.

Lambda functions automatically capture any variables from the surrounding scope that are used within
the lambda expression. Captured variables are copied into the lambda function (in fact, they become
member variables inside a anonymous object), which means that they behave as if they were passed to
a function (e.g. values are copies, classes and actors are references). This is illustrated in the
example below, where the lambda function will remember the value of the variable `outside` even when
it goes out of scope:

```
Int outside = 20;
var add = (Int x) => x + outside;
```

Since captured variables are copied to the lambda functions, values can be modified without
affecting the surrounding scope. This can be used to create a counting function:

```
fn->Int counter() {
    Int now = 0;
    return () => now++;
}

void foo() {
    var a = counter();
    var b = counter();

    a.call(); // => 0
    a.call(); // => 1
    b.call(); // => 0
}
```

This syntax is implemented in `lang:bs:lambda.bs`.


Null and maybe
---------------

By default, reference variables (class or actor types) can not contain the value null. Sometimes,
however, it is useful to be able to return a special value for special condition. For this reason,
Storm contains the special type `Maybe<T>`. This type is a reference, just like regular pointers,
but it may contain a special value that indicates that there is no object (null). In Basic Storm,
the shorthand `T?` can be used instead of `Maybe<T>`.

`Maybe<T>` can not be used to access the underlying object without explicitly checking for
null. Null checking is done like this:

```
Str? obj;
if (obj) {
    // In here, obj is a regular Str object.
}
```

Automatic type conversions are done when calling functions, so you can call a functions taking
formal parameter of type `T?` with an actual parameter of type `T`.

At the moment, automatic type casting does not always work as expected, mainly when the compiler has
to deduce the type from two separate types. For example in the if-statement in `fn` below:

```
Str? fn2() {
    "foo";
}

Str? fn() {
    if (condition) {
        fn2();
    } else {
        ?"bar";
    }
}

```

In this case, we need to help the compiler deducing the return value of the if-statement. The
compiler sees the types `Str` and `Str?`, and can fail to deduce the return type. This happens
especially when a possible type is neither `Str`, nor `Str?`. To help the compiler, add a `?` before
`"bar"` to cast it into a `Str?`.

`Maybe` instances are initialized to null, but sometimes it is desirable to assign a null value to
them after creation. In Basic Storm, there are two ways of doing this. Either by assigning a newly
created `Maybe` instance, or using the special keyword `null`. `null` can be a part of an
expression, but it has no meaning unless it is used in a context where it can be automatically
casted to a `Maybe` type. For example:

```
Str? v = "Hello, World";

// Set 'v' to null.
v = null;

// Equivalent, but more verbose.
v = Str?();
```

Note that this has no meaning:

```
// 'v' will have the type 'void' here. 'null' does not have 
// a type unless it can be automatically casted to something usable.
var v = null;
```


Loops
-------

All kinds of loops that work in C++ work in Basic Storm, as well as the keywords `break` and
`continue`. However, there are some differences.

Firstly, the `do`, and the `while` loop has been combined into one. To illustrate this, consider the
following two examples:

```
while (a < 10) {
    a++;
}
```

```
do {
    a++;
} while (a < 10);
```

They are using the same keywords, and approximately in the same order. Furthermore consider this
fairly common pattern:

```
int a = 0;
while (true) {
    cout << "Enter a positive integer: ";
    cin >> a;
    if (a > 0)
        break;
    cout << "Not correct. Only positive numbers." << endl;
}
```

As we can see here, resort to using a `while(true)` loop with break rather than using the
condition. If we would have used the condition, we would have to repeat other parts of the
expression. Because of this, Basic Storm allows you to write:

```
Int a = 0;
do {
    print("Enter a positive integer: ");
    a = readInt();
} while (a <= 0) {
    print("Not correct. Only positive numbers.");
}
```

This works exactly as the C++ example above. Any sequence of statements can be omitted, and blocks
can be replaced with a semicolon. From this, we can form all loops in C++ (except the `for` loop),
but we also gain an alternative to the `while (true)` construct:

```
do {
    // code repeated forever
}
```

The do-while loop in Basic Storm has a bit special scoping rules. Normally, scope is resolved
lexicographically, but both blocks in a do-while loop are considered to be one block. Look at the
code below, where we declare a variable in one block and use it in the other:

```
Int i = 1;
do {
    Int j = 2*i;
} while (j < 20) {
    i += j;
}
```

The for-loop
-------------

The for loop from C++ is also present in Basic Storm. It works just like in C++:

```
for (Int i = 0; i < 20; i++) {
    // Repeated 20 times.
}
```

There is also a variant that iterates through a container or a range:

```
Int[] array = [1, 2, 3, 4];
for (x in array) {
    // Repeated for each x in the array.
}
```

This is equivalent to:

```
Int[] array = [1, 2, 3, 4];
for (var i = array.begin(); i != array.end(); ++i) {
    var x = i.v;
    // Repeated...
}
```

When the container's iterator associates a key to each element, write `for (key, value in x)` to
access the key in each step as well. `key` is extracted using the `k` member of the iterator, while
`value` is extracted using the `v` member. See [iterators](md://Storm/Iterators.md) for more
information.


Exceptions
-----------

Exceptions are thrown using the `throw` keyword:

```
throw RuntimeError("Something went wrong!");
```

The right-hand side can be any valid Storm expression, but thrown exceptions must inherit from
`core.Exception`, either directly or indirectly. New exceptions can be declared in Basic Storm just
like any class:

```
class MyException extends Exception {
    void message(StrBuf to) : override {
        to << "My error";
    }
}
```

This exception overrides the abstract function `message` to print a custom error message. If we want
to include a stack trace, we need to instruct the system to collect a stack trace when the exception
is created by calling the `saveTrace` function in the constructor:

```
class MyException extends Exception {
    init() {
        init();
        saveTrace();
    }
    void message(StrBuf to) : override {
        to << "My error";
    }
}
```

Note that we don't need to print the stack trace anywhere. That is handled by the default `toS`
implementation in `Exception`.

Exceptions are caught using a `try` block:

```
try {
   throw MyException();
} catch (MyException e) {
    print("My exception: ${e}");
} catch (Exception e) {
    print("Generic exception: ${e}");
}
```

The code inside the `try` part of the block is executed as normal. However, if an exception is
thrown inside the block, execution immediately proceeds in one of the `catch` blocks. The system
searches the `catch` blocks in the order they appear in the source code, and executes the first one
where the declared type matches the actual type of the thrown value. In the example above, the first
case would be selected, as we throw an instance of `MyException`. Any other exceptions would be
caught by the second `catch` block. It is possible to omit the variable name (`e` in this case) if
the caught exception is not needed in the `catch` block.
