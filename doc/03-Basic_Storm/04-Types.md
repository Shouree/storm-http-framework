Types
======

Type declaration also follows closely to the C++ declarations. However, since Storm has three
distinct kind of types, the syntax has been extended a bit.

* __Value:__

  `value Foo { <contents> }`

  or

  `value Foo extends T { <contents> }`

* __Class:__

  `class Foo { <contents> }`

  or

  `class Foo extends T { <contents> }`

* __Actor:__

  `class Foo on T { <contents> }` (T is the name of a thread)

  or

  `class Foo on ? { <contents> }`

In these cases, contents is a list of member functions and member variables. Member functions are
declared just like regular functions, except that they may not include a thread directive, instead
they inherit the thread directive from the enclosing type. Member variables are declared just like
local variables in a function (i.e. `<type> <name>`). Initializing variables at the declaration site
is not supported yet. There are also no visibility declarations yet, everything is public at the
moment.

Note that it is not possible to inherit from a class _and_ declare the class as an actor. If the
super class was an actor the new class will be an actor as well, otherwise it will not.

Storm allows nested type definitions. These work like in C++, which means that a type declared
inside another type will be able to access all private parts of the outer type. Aside from that,
Storm does not consider them to have any special relation to each other.

Constructors
--------------

Types may have constructors. If no destructor is declared, Basic Storm generates a constructor for
you. Constructors are declared by the special syntax:

`init(<parameters>) { <body> }`

or, if the constructor represents a type cast that can be performed implicitly:

`cast(<parameters>) { <body> }`

The constructor is special. Because it is in charge of creating an object, no object exists at the
start of the constructor. Because of this, the variable `this` is not available until the object has
been initialized. The initialization is similar to initializer lists in C++, but allows executing
arbitrary code before, during, or after initialization. The initialization is performed in two
steps. First, the constructor of the super class is called as follows:

`super(<parameters>);`

After calling `super` in this manner (which has to be done at the top-level, not inside any other
blocks such as if-statements), the `this` parameter is available in the function. If no superclass
is present, this step can be omitted. However, as only the super class is initialized, `this` will
have the type of the parent class and not the current class until the object is fully
initialized. The second stage of the initialization is done using the init block as follows:

`init { <init-list> }`

Where `<init-list>` is a list containing initializers for zero or more of the member variables in
the type. Member variables not present in the list are initialized using their default constructor
(Basic Storm does not initialize things to `null`, use `Maybe<T>` or `T?` for that). `<init-list>`
is a list of assignments (`<name> = <expr>`) or constructor calls (`<name>(<params>)`) separated by
semicolons (`;`). These initializers are always executed in the order they are declared, regardless
of the original order of the initialized members. Any members that are initialized implicitly are
initialized before the explicitly initialized members.

For convenience, if no code needs to be executed between the call to `super` and the init block, all
initialization can be done at once by the `init` block. If the superclass' constructor has not been
called when the init block is reached, the init block will call the superclass'
constructor. Parameters to the constructor can be provided as follows:

`init(<parameters>) { <init-list> }`

Note that `init {}` and `init() {}` are not equivalent. The first one does not explicitly try to
call the constructor of the superclass, and is allowed after an explicit `super` call. The second
one indicates that the constructor should be called, and is therefore not allowed after an explicit
call to `super`. If no explicit initialization is necessary, both `super` and `init` (or one of
them) can be omitted.

Finally, instead of initializing the object using `super` and `init`, the constructor may instead
delegate initialization to another constructor entirely. This is done as follows:

`self(<parameters>)`

This causes the suitable constructor to be called. As with `super` and `init`, the variable `this`
is not available until after `self` has been called.

The constructor acts a little special when working with actors that have not been declared to be
executed on a specific thread (using the `on ?` syntax). These constructors need to take a `Thread`
as their first parameter (you do not have to give it a name), and the first parameter is always
automatically passed to the parent constructor. This is needed to make it possible for Storm to
ensure that even the constructor of the actor is running on the given thread to the not-yet created
actor.


Actors
--------

As mentioned in [the Storm documentation](md://Storm/Type_system), actors behave slightly
differently compared to classes. Each actor is associated with a thread, either statically or
dynamically upon creation. Storm will then make sure that all code in the actor is executed by the
appropriate thread. Furthermore, Storm ensures that no data is shared between different threads.

In order to ensure these properties hold, Storm will examine each function call and perform a
*thread switch* if necessary. A thread switch involves sending a message to another thread (using
`core:Future<T>` to wait for the result) and making deep copies of all parameters and return values
to avoid shared data. This means that one needs to be a bit careful when working with actors, since
a class that is passed through a function declared to be executed on another thread will be copied
along the way, thereby breaking the by-reference semantics that one might expect. This is the main
reason why the `==` operator for classes compares values rather than references.

This mechanism also introduces difficulties when dealing with member variables inside actors. In
order to avoid data races and shared data, accesses to these kind of variables need to be
synchronized, and data needs to be copied just like function calls. Basic Storm implements this
using *thread switches*, just like for function calls. A small `read` function is used to perform
the variable read on the appropriate thread with copying as appropriate. However, this solution
means that accessing variables on other threads will not behave as regular accesses. In particular,
due to the copying it is not possible to modify the variable in this manner. Basic Storm will detect
this kind of situation if the variable is immediately assigned to (e.g. `foo.x = 8`), but will not
detect in more complex cases such as `foo.x++` or `foo.x.z = 10` (where `foo.x` is the access that
requires a thread switch).


Function declarations in types
-------------------------------

Declaring functions inside types in Basic Storm is mostly the same to declaring functions at the
top-level. However, functions declared inside a type will become member functions, which means that
they receive a hidden first parameter describing the instance being operated on. In Basic Storm,
this is mostly equivalent to a global function with the first parameter named `this`. The name
`this` is special in Basic Storm, since the variable named `this` will automatically be inserted as
an implicit first parameter to functions of the form `foo(bar, baz)` (as opposed to `bar.foo(baz)`),
just as one would expect for member functions.

Aside from that, member functions can be declared as assignment functions by replacing the return
type with the keyword `assign`.

Aside from assignment functions, Basic Storm provides the decorators `final`, `abstract`, `override`
and `static` that tell Storm your intentions with regards to inheritance. These keywords are
specified after the parameter list of the function but before the function body like this:

```
class Foo {
    Int foo() : final { 10; }
}
```

These keywords correspond to the function flags `fnFinal`, `fnAbstract`, `fnOverride` and `fnStatic`
respectively, which are documented [here](md://Storm/Type_system). If a function is marked
`abstract`, it is possible to omit the function body entirely. If the function is ever called (for
example by `super:foo`), it will throw an appropriate exception. For example:

```
class Foo {
    Int bar() : abstract;
}
```

To summarize, the following decorators are available for member functions in addition to those
available for [nonmember functions](md://Storm/Functions):

* `final`: Indicates that subclasses may not override this function.
* `abstract`: Indicates that subclasses must override this function.
* `override`: Indicates that this function should override a function in the superclass.
* `static`: Indicates that this function does not operate on an instance of the enclosing class, much like
  a nonmember function.


Decorators for types
---------------------

Types may also be decorated, just like functions. Actually, the keywords `extends` and `on` used
above are implemented as decorators. Decorators for classes are more general than for functions; a
decorator is the name of a function which may modify the class in some way (or a special form
defined with custom grammar). As such, it is fairly easy to create and use decorators for
classes. Just create a function taking a type to modify as a parameter, and make sure it is visible
where it is used.

Decorators for types are specified, much like for functions, after a colon (`:`) following the name
of the type. Multiple decorators are separated by commas. For example:

```
class Foo : extends Bar, persist {}
```

Here, the class `Foo` is decorated with `extends Bar` and `persist`. The syntax seen at the top of
this page, where only one of the special decorators are used, are just a special case of the general
decorator syntax. As such, they can be written as follows as well:

`value Foo : extends T { <contents> }`

`class Foo : extends T { <contents> }`

`class Foo : on T { <contents> }`

`class Foo : on ? { <contents> }`

Note that the general form has to be used when multiple decorators are to be applied. It is not
possible to declare a type like this:

`class Wrong extends T, persist {}`


Enums
-----

Enums in Basic Storm are declared much like in C++, except that no trailing semicolon is needed:

```
enum MyEnum {
    a,
    b
}
```

This creates an enum named `MyEnum` that has two values, `a` and `b`. By default, values are
numbered from zero and onwards, but this can be changed by adding an element as `a = 5` for
example. Note that only decimal and hexadecimal literals are allowed as initializers. Expressions
are not supported. Any values following the explicitly numbered value will continue from the
explicit number as follows:

```
enum MyEnum {
    a, // will be 0
    b = 5, // will be 5
    c  // will be 6
}
```

Enums are represented as a 32-bit integer, which can be inspected using the member `v` of the
type. The enum also contains a constructor to explicitly cast to the enum type from a `Nat`.

Basic Storm also supports "bitmask enums", i.e. a set of named booleans stored in a single 32-bit
integer. These are declared by adding the `bitmask` keyword as such:

```
enum MyBitmask : bitmask {
    a, // will be 0x01
    b, // will be 0x02
    c = 0x10, // will be 0x10
    d  // will be 0x20
}
```

When declared as a bitmask, the values automatically assigned to values start at 1 and double each
time. Furthermore, the set of bits can be queried using the following operators:

- `a + b` Combine two enums, creating the union.
- `a - b` Remove any set bits from the `b` side from the `a` side.
- `a & b` Check which bits are set in both `a` and `b`.
- `a.has(b)` Same as `&`.

