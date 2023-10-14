Function Objects
================

Function objects (or function pointers) are represented by the type `core:Fn<R, A, B, ...>`. The
parameters to the type correspond to the return value and the parameters of the type. The first
parameter, `R`, is the return value, and any remaining parameters are the parameters to the
function. As such, `core:Fn` requires at least one parameter (which may be `void`).


Syntax
------

As mentioned [previously](md:../Names), Basic Storm provides a short-hand syntax for function
pointers:

```bsstmt:placeholders
fn(<params>)-><result> f1; // Equivalent to Fn<<result>, <params>>
fn(<params>)->void     f2; // Equivalent to Fn<void, <params>>
fn-><result>           f3; // Equivalent to Fn<<result>>
fn->void               f4; // Equivalent to Fn<void>
```

For example, both declarations of `f1` and `f2` are equivalent:

```bsstmt
fn->Int           f1;
Fn<Int>           f1;

fn(Int, Int)->Nat f2;
Fn<Nat, Int, Int> f2;
```


Pointers to Free Functions
--------------------------

Basic Storm allows creating pointers to functions using the `&` operator. This is illustrated in the
example below:

```bs
Nat toNat(Int x) {
    x.nat;
}

Nat toNat(Long x) {
    x.nat;
}

fn(Int)->Nat pointer() {
    // Create a pointer to the function that takes an Int as a parameter.
    return &toNat(Int);
}
```

Since Storm supports function overloading, Basic Storm needs to deduce which overload of the
function it shall create a pointer to. As can be seen above, this is done by specifying the types of
the formal parameters of the function in parentheses after the function name, similarly to the
function declaration.

In situations where Basic Storm is able to deduce the expected type of the function pointer
variable, it is possible to skip the parameter list. For example, in the example above it would be
enough to write `return &toNat;` since the return type of the function is `Fn<Nat, Int>`. One
example where it is *not* possible is below:

```bsstmt
var x = &toNat; // Error: unable to deduce the type of 'x'.
```


Pointers to Member Functions
----------------------------

Since Basic Storm supports uniform function call syntax (i.e. `x.f(y)` and `f(x, y)` are
equivalent), it is possible to create pointers to member functions in a similar way:

```bs
class MyClass {
    void member(Int x) {}
}

fn(MyClass, Int)->void pointer() {
    return &MyClass:member;
}
```

Note that this approach creates a pointer to the member itself. The pointer is not associated with a
particular instance of the object.


If the first parameter to a function is a class- or actor type, it is possible to bind the parameter
to a particular value when creating the function pointer. This is done using the `.` operator as
follows:


```bs
class MyClass {
    void member(Int x) {}
}

fn(Int)->void pointer() {
    MyClass cls;
    return &cls.member;
}
```

Note that the return type from `pointer` is now `fn(Int)->void` and not `fn(MyClass, Int)->void`.
This is because the first parameter is provided by the function object, and does thus not need to be
passed as a parameter.

Due to uniform function call syntax, binding the first parameter is not limited to member functions.
This can be done also for free functions, as long as their first parameter is a class- or actor
type.

As before, in both cases above, it is always possible, and sometimes required, to specify the list
of formal parameters in order to disambiguate between different overloads.



Lambda Functions
----------------

Lambda functions are anonymous functions that can be created inside other functions easily. A lambda
function evaluates to a suitable function object. It has the following structure:

```
(<formal parameters>) => <expression>;
```

Where `<formal parameters>` is a list of formal parameters to the function, and `<expression>` is
the expression that produces the result of the function. As with other parts of Basic Storm, the
expression may be a block. For example, a lambda function that adds two numbers can be defined as
follows:

```bsstmt
fn(Int, Int)->Int x = (Int a, Int b) => a + b;
```

In situations where Basic Storm is able to infer the type of the pointer, it is possible to omit the
types of the formal parameters. For example, when passing the lambda function as a parameter to a
function, or when assigning it to a variable with a known type:

```bsstmt
Array<Int> x = [1, 2, 3, 4];
// Sort in reverse order:
x.sort((a, b) => a > b);
```

Lambda functions automatically capture any variables required from the surrounding scope that are
used within the lambda expression. Captured values are copied into the lambda function (in fact,
they become meber variables in an anonymous object). This means that they behave as if they were
passed to a function (i.e. values are copied, classens and actors are references). This is
illustrated in the example below, where we utilize that copied local variables are stored inside an
anonymous object:

```bs
fn->Int makeCounter() {
    Int now = 0;
    return () => now++;
}

void main() {
    var a = counter();
    var b = counter();

    a.call(); // => 0
    a.call(); // => 1
    b.call(); // => 0
}
```

Since `Int` is a value type, any changes to the `now` variable in the lambda function created in the
`makeCounter` function are not visible in the original variable, and vice versa. This would not be
the case if `now` was a class- or actor type since they are reference types. We can see the effects
in the following example program:

```bs
void byvalue() {
    Int now = 0;
    fn->Int lambda = () => now++;

    now++;
    lambda.call();  // => 0, the lambda has a separate copy of 'now'
    print(now.toS); // => 1, the call to the lambda does not affect 'now'
}

void byreference() {
    Nat[] data;
    fn->Nat lambda = () => { data << data.count; data.count; };

    data << 5;
    lambda.call();   // => [5, 1] - data is shared by reference
    print(data.toS); // => [5, 1] - data is shared by reference

    data = Nat[];    // This assignment only affects 'data', the reference
                     // inside the lambda is not modified.
    lambda.call();   // => [5, 1, 2]
    print(data.toS); // => []
}
```

**Note:** while the `this` pointer can be captured in lamdba functions, it currently collides with
the `this` pointer of the lambda function. To work around this, it is necessary to create a variable
with a different name:

```bs
class MyClass {
    Int member;

    void fn() {
        // Will not work, would capture 'this':
        var lambda = (Int x) => member + x;

        // Instead, create a local variable and capture that:
        var me = this;
        var lambda = (Int x) => me.member + x;
    }
}
```

Lambda functions are implemented entirely in Basic Storm. The implementation is in the file
`lambda.bs` in the package `lang:bs`.


Calling Functions
-----------------

Function objects contain a `call` member that invokes the function. For example:

```bsstmt
var x = (Int a, Int b) => a + b;
print(x.call(2, 3).toS); // Prints 5
```

If the function pointer is bound to a function that is associated with a thread other than the
thread that called the `call` member, then a message will be sent to the thread. This means that all
parameters will be copied, along with the return value from the function. Note that in contrast to
other situations in Basic Storm, this decision is made dynamically by the function object, rather
than statically. As such, function objects provide a way to avoid sending messages in some
situations:

```bs
thread T1;
thread T2;

void toCall(MyClass x) on T1 {}

void intermediary(MyClass x, fn(MyClass)->void ptr) {
    // Will always send a message: this function can
    // be executed on any thread, but 'toCall' needs
    // to run on T1.
    toCall(x);

    // Might send a message depending on the thread
    // we are running on.
    ptr.call(x);
}

void main1() on T1 {
    // Calling from here will *not* make a copy when
    // calling through the function pointer.
    intermediary(MyClass(), &toCall);
}

void main2() on T2 {
    // Calling from here *will* make a copy when
    // calling through the function pointer.
    intermediary(MyClass(), &toCall);
}
```

As can be seen in the comments, when `main1` is executed, `MyClass` will be copied once in
`intermediary` when the regular call occurs. However, not when the function pointer is used since it
makes a dynamic decision based on the current thread. When `main2` is executed, however, both calls
in `intermediary` will send a message and thereby copy `MyClass`.
