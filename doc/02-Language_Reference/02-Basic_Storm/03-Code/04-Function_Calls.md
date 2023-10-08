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
