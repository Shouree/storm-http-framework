Function Calls
==============

Function calls are written as `<name>(<parameters>)`, similarly to many other languages. One
difference to other languages is that the parentheses around the parameters are optional whenever no
parameters are specified. In other words, the call `x()` and `x` are equivalent.

Perhaps the easiest way to explain the reason for this is to describe how Basic Storm treats
function calls. Whenever an identifier is seen (e.g. `foo`, `foo()`, or `foo(a, b)`), it creates a
`NamePart` with the identifier that was found. If any parameters were specified, the `NamePart` is
filled with the types of the parameters as well. Basic Storm then looks up the name as described
[here](md:../Names). Finally, it inspects whatever named entity was returned by the lookup and acts
accordingly. If the entity was a function, the function is called. If it was a variable, the value
of the variable is returned, and if it is the name of a type, the constructor is called.

The fact that `x` and `x()` are the same is useful when implementing something that should look like
a member variable. Perhaps, a read-only member variable. Furthermore, the existence of assign
methods allows seamless migration from a member variable into a getter and a setter function.
To illustrate this idea, consider the example below, where the variable `value` is public:

```bs
class MyClass {
    Int value;
}

void useClass() {
    MyClass v;
    print(v.value.toS);
}
```

Consider that you later on might want to make `value` into a private member that is accessed by a
getter and a setter. This can be achieved like this to avoid changing all usages of the value:

```bs
class MyClass {
    private Int myValue;

    Int value() { myValue; }

    assign value(Int newValue) { myValue = newValue; }
}

void useClass() {
    MyClass v;
    print(v.value.toS);
}
```


Furthermore, as described [here](md:../Names), Basic Storm implements a custom name lookup to make
the expressions `fn(x)` and `x.fn()` behave almost equivalent. As such, this allows extending the
percieved interface of types from other packages. To illustrate this, consider the example below:

```bs
class MyClass {
    Int data;

    // Function that is a part of the original interface.
    void a() {
        print("original: ${data}");
    }
}

// Function in another package that extends the interface of MyClass.
void b(MyClass this) {
    print("extension: ${data}");
}

// Function using the class, in yet another package.
void useClass() {
    MyClass x;
    // Note how it appears as if both 'a' and 'b' are members
    // of MyClass here:
    x.a();
    x.b();
}
```

Note that it is possible to name parameters `this` in free functions. This makes it possible to
access members of the type of that parameter *as if* the function was a member function. Of course,
this does not allow access to any private members.


While `fn(x)` and `x.fn()` are generally equivalent, there is a subtle difference in their behavior
when there are multiple possible alternatives available. This is illustrated below:

```bs
class MyClass {
    void fn() {
        print("member");
    }
}

void fn(MyClass x) {
    print("free function");
}

void main() {
    MyClass c;
    c.fn(); // prints "member"
    fn(c); // prints "free function"
}
```


Threads
-------

Storm and Basic Storm is aware of threads in the system, and ensures that functions that need to
execute on a particular thread are executed on that thread. As such, when a member of an actor is
called, or when a function that is marked with `on T` is called, Basic Storm sends a message to the
appropriate thread rather than calling the function directly. This is often transparent to the
programmer. But since sending a message means that parameters are copied, it is visible in some
situations.

To behave in a predictable manner, Basic Storm determines statically when it is necessary to send a
message rather than calling the function directly. As such, a message is sent when:

- The called function is declared to be executed on a different thread than the current function.
  This applies both if the current function is declared to run on any thread and calls a function
  that is declared to run on a specific thread, or when both are associated to different threads.

- The called function is a part of an actor declared as `on ?` (i.e. it is not statically known
  which thread it is associated with). The exception to this rule is when the called function is a
  member of the same instance as the current one (i.e. for calls like `this.f()` whether `this` is
  explicit or not).

There is one exception to the two rules above: calls through the function pointer interface. Since
the function pointer interface does not know the static properties of the call site, it determines
dynamically whether or not to send a message.

As mentioned, this is visible in certain situations since classes appear to have by-value semantics.
We can illustrate this by passing a class as a parameter to a function declared to run on a thread,
and compare the behavior of calling it from the same thread and from a different thread:

```bs
class MyClass {
    Int x;
}

MyClass other(MyClass x) on Compiler {
    x.x = 20;
    x;
}

void sameThread() on Compiler {
    MyClass original;

    // Note: no message, same thread.
    MyClass returned = other(local);
    // Changes are visible as expected:
    print(original.x.toS); // => 20
    // The same object is returned:
    print(original is returned); // => true
}

void noThread() {
    MyClass original;

    // Note: the call to 'other' causes a message to be sent.
    MyClass returned = other(local);
    // Since 'original' was copied, we won't se changes here:
    print(original.x.toS); // => 0
    // Also, 'returned' is a copy, so the objects are not the same:
    print(original is returned); // => false
    // But we get the changes in our returned copy.
    print(returned.x.toS); // => 20
}
```

The function `sameThread` is declared to run on `Compiler`, just like `other`. As such, the
function call behaves as usual. The modifications to the class parameter are visible outside of the
function, and the identity of the returned object is the same.

However, when we call `other` from `noThread`, which is declared to run on any thread, the
semantics differ. Since Basic Storm does not statically know what thread `noThread` will be
executed on, it will always send a message when calling `other` (i.e. even if `noThread` was called
from the `Compiler` thread). This means that non-actor parameters to `other` are copied, and
therefore changes to the variable `original` are not visible in `noThread`. Furthermore, the
returned object is a copy, so `returned` is not the same object as `original` in this case.


Asynchronous Calls
------------------

Whenever Basic Storm needs to send a message to another thread, the default behavior is to wait for
the the other thread to complete execution of the function. It is, however, possible to continue
execution in the original thread in parallel, and optionally wait for the result later.

This is achieved by using the keyword `spawn` before a function call. The `spawn` keyword makes two
changes to the default behavior. First, it always sends a message instead of calling the function,
even if the function could be called directly in the current context. Secondly, it causes the
expression to evaluate to `Future<T>` instead of `T`. The `Future` object has a member `result` that
can be called to wait for, and acquire the result. It is also possible to call `detach` to discard
the result entirely.

As such, concurrent execution can be achieved as follows:

```bs
void fn(Str title) {
    for (Nat i = 0; i < 10; i++) {
        print("${title}: ${i,2}");
	sleep(1 s);
    }
}

void main() {
    Future<void> a = spawn fn("A:");
    Future<void> b = spawn fn("B:");

    // Wait for both to terminate:
    a.result();
    b.result();
}
```

**Note:** Remember that Storm utilizes [cooperative scheduling for each OS
thread](md:../../Storm/Threading_Model). This means that without the call to `sleep`, the two calls
to `fn` may execute one after the other. In this case, the call to `print` requires sending a
message to the `Compiler` thread, which yields the current thread without `sleep`, however.
