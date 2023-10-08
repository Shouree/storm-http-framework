Expressions
============


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
