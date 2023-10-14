Functions
=========

Functions in Basic Storm have the following form:

```
[<visibility>] <return type> <name>(<parameters>) [: <options>] {
    <body>
}
```

The part `<return type>` is the name of the type returned from the function. The `<name>` is a
single name part (without parameters) that denote the name of the function. Finally, `<parameters>`
is a comma separated list of formal parameters. Each parameter consists of the name of the type of
the parameter followed by the name of the parameter in the function.

The optional `<visibility>` is described in detail in the section [definitions](md:).


For example, a function that adds two integers may be defined as follows:

```bs
Int myAdd(Int a, Int b) { a + b; }
```

Functions that are declared inside of types are considered to be member functions. These functions
will be given an implicit first parameter named `this` that is a reference to an instance of the
type in which the function is declared. To illustrate this, consider the function `set` in the class
below:

```bs
class MyInt {
    Int value;

    void set(Int newValue) {
        value = newValue;
    }
}
```

The `set` function here takes two formal parameters. The first one is of the type `MyInt` and it is
named `this`. The second one is of type `Int` and has the name `newValue` as specified. As such, the
function `set` could (almost) equivalently be declared outside the class as follows:

```bs
class MyInt {
    Int value;
}

void set(MyInt this, Int newValue) {
    value = newValue;
}
```

This illustrates how the function parameters are transformed by Basic Storm. The example also
illustrates that it *is* possible to name a parameter `this` in Basic Storm. Furthermore, doing so
allows referring to variables in the type implicitly, just as if the type would have been a member.
In this case, the variable `value` refers to `this.value` since a `this` variable is available.

Thus, the only difference between the two versions above are related to scoping. First, as described
in the section on [name lookup](md:../Names), the `set` function that is a member of `MyInt` will be
prioritized over the free version if it is called as `MyInt().set(5)`, and the free version will be
prioritized if it is called as `set(MyInt(), 5)`. The second difference is that the free version of
`set` is not able to access any private or protected members of `MyInt`.


Assign Functions
----------------

In Basic Storm, it is possible to mark functions as *assign functions* by specifying the return type
as `assign`. This causes the return type to be void, and allows the function to appear in the left
hand side of an assignment. If this happens, Basic Storm transforms the assignment into a call of
the function instead. Since Basic Storm does not require parentheses when calling functions with
empty parameter lists, this allows emulating a public member using a plain function as a getter, and
an assign function as a setter. This can be done for both member functions and free functions. To
illustrate this, consider the code below:

```bs
class MyClass {
    private Nat myValue;

    // Getter function.
    Nat value() { return myValue; }

    // Setter assignment function.
    assign value(Nat v) { myValue = v; }
}

// Usage of assign functions:
void use() {
    MyClass x;

    // Calls MyClass:value().
    Nat tmp = x.value;
    // Calls MyClass:value(Nat)
    x.value = tmp;
}
```



Options
-------

Zero or more *options* can be applied to functions. This is done by appending a colon (`:`) followed
by a comma-separated list of options to the function definition. The following options are available
by default, but libraries may provide additional options:


### Options for All Functions

Additional options can be provided by extending the rule `lang:bs:SCommonOption`. The ones provided
by Basic Storm are:

- `pure`

  Marks the function as *pure*. This means that the system may assume that the function has no
  observable side-effects, and that the output from the function only depends on the inputs to the
  function.

  Currently, this marker is only used to determine when a copy-constructor is trivial with respect
  to the calling convention. There are, however, plans to utilize the `pure` modifier to perform
  constant folding in the future, which means that calls to `pure` functions may be elided, and the
  function body might be executed entirely at compile time rather than at run time.

- `inline`

  Marks the function as *inline*, which in Basic Storm means that it is always inlined. As such,
  this modifier is only suitable to use for short and simple functions.


### Options for Free Functions

Additional options can be provided by extending the rule `lang:bs:SFreeSpecialOption`. The ones
provided by Basic Storm are:

- `on <thread>`

  Specifies that the function should always execute on the thread named `<thread>`. If the function
  is called from another context, the system will send a message to the appropriate thread to ensure
  that the call happens by the specified thread. This option is special, and the colon may be
  omitted if the `on` option is the only option present.


### Options for Member Functions

Additional options can be provided by extending the rule `lang:bs:SMemberOption`. The ones provided
by Basic Storm are:

- `static`

  Makes the function *static*, which means that it will not be given an implicit `this` parameter.
  It may therefore not access members of the surrounding type without explicitly acquiring an
  instance in some manner.

- `abstract`

  Makes the function *abstract*, which means that it is not necessary to provide an implementation
  for the function in this type. Any type that contains abstract functions are not possible to
  instantiate.

- `final`

  States that the function is not possible to override in derived classes.

- `override`

  Asserts that the function should override a function in the parent class. An error is raised if
  this is not the case.

