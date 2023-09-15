Type system
============

The type system in Storm is inspired from languages like C++, but modified to suit the threading
model in the compiler while also trying to make things simple. Both when declaring and using types.
There are three distinct kind of types in Storm: values, classes and actors. Each of these are
explained below.

* **Values**

  Values are intended for small types, where the overhead of doing heap-allocations are large compared
  to the content of the object. Values are always allocated on the stack, and have value semantics
  when passed to and from functions.

* **Classes**

  Classes work like in Java. They are always heap-allocated and storage is reclaimed using garbage
  collection, and since pointers are passed to functions, they look like references. Storm generally
  looks at classes more like values than Java, for example: the `==` operator will compare the
  objects and not just see if the two objects are the same object (this is not implemented
  yet!). However, it is up to each language to decide on the exact semantics of equality checks. All
  classes either directly or indirectly inherits from the `Object` class

* **Actors**

  The final kind of types in Storm are actors. Actors are intended to be the pure by-reference type
  in Storm. Actors are, as classes, always allocated on the heap. However, an actor is also
  associated with a thread, and the compiler (languages may choose to ignore this at the moment)
  will ensure that any of the member functions in the actor are executed on that specific thread. All
  actors inherits either directly or indirectly from `TObject`. Note that `TObject` and `Object` are
  not related in the Storm type system.

These three kind of types provides steps from pure values (value) to pure references (actor), where
classes are something in between. To explain the reasoning behind these three kind of types, we have
to take a quick look at the threading model. The goal of the threading model in Storm is to ensure
that no data is shared between different threads (and to help the programmer keep track of what runs
where, we will see this later on). To accomplish this, Storm takes an approach similar to the actor
model. Each thread is an actor, and when calling a function on another thread a message is sent to
the other thread. To ensure that no data is shared, anything that is sent in a message has to be
copied. Values and classes fall into this category, but actors are an exception to this rule, as
actors are only accessed through messages, which are thread safe anyway.

Based on this information, we can see the type system from two different perspectives: when we are
working from within a single thread, we can get value semantics by using values, and reference
semantics using classes (or actors). When we observe the behavior when types are sent through a
message, things are slightly different. In this case, both values and classes seem like they have
by-value semantics (any changes from the other thread are not visible) while actors still have
by-reference semantics. This is also the reason why classes in Storm behave more like values than in
Java.

Function overriding
---------------------

Storm supports function overriding in classes and actors. Overriding functions causes vtable-based
dispatch to be used, but unlike e.g. C++, Storm will keep track of where the vtable is required and
only use it if it is really necessary. Thus, you will not pay the cost of overriding functions
unless you use it.

A function in a derived class does not have to match a function in the parent class exactly. It is
possible to accept wider types (i.e. less specific) in the derived class. However, this means a
function could override two functions in a parent class, which is not allowed. This happens if, for
example, the parent class contains the functions `add(Str)` and `add(Url)`, and the derived class
contains the function `add(Object)`. However, if the parent class would also contain an exact match,
(`add(Object)` in this case) as well, the exact match is preferred over inexact matches. This
restrictive behaviour is to reduce the unintentional surprises in the overriding behaviour.

Intentions regarding function overriding can be asserted by Storm, much like in other
languages. This is done by setting flags on the relevant functions (using
`Function.make(FnFlags)`). The following flags are available in this regard:

* `fnFinal` asserts that this function should not be overridden in derived classes.
* `fnAbstract` asserts that this function is abstract and has to be overridden in a derived class.
* `fnOverride` asserts that this function is supposed to override a function in a parent class.


Cloning
---------

As mentioned above, Storm has to be able to clone values and classes whenever they are sent as a
message. Cloning comes in two types: deep and shallow cloning. Shallow cloning is implemented, just
like in C++, by the copy constructor of the type. In Basic Storm, a copy constructor is
automatically generated for you if you do not declare it yourself.

Deep cloning clones an entire object graph, and manages arbitrary graphs, even graphs containing
cycles and shared objects. The deep cloning is implemented using the compiler-generated `clone`
method in `core`. The compiler generated `clone` looks like this (in C++-like syntax):

```
T clone(T v) {
    T tmp(v);
    CloneEnv env;
    tmp.deepCopy(env);
    return tmp;
}
```

It first calls the copy-constructor, then creates a built-in object called `CloneEnv` and finally
calls the `deepCopy` member. `deepCopy` is also automatically generated if you are using Basic
Storm, and this function makes sure to either clone or call `deepCopy` on any member variables of
that type. The `CloneEnv` remembers any objects that have been cloned, so that we can correctly
handle any cycles or shared parts of the object graph.


The clone methods are implemented like this:
```
void deepCopy(CloneEnv env) {
    super.deepCopy(env);
    a = clone(a, env);
    // ...
}

T clone(T object, CloneEnv env) {
    T result = env.cloned(object);
    if (!result) {
        result = new T(object);
        result.deepCopy(object, env);
    }
    return result;
}
```

Note that cloning an actor simply returns the actor itself, since actors are supposed to reference a
single instance.

Type requirements
------------------

Any members that are required by Storm will be generated automatically by most languages (such as
Basic Storm), but this must be done manually when working in C++.

Values and classes need the following members:
* Copy constructor
* `deepCopy(CloneEnv)` - making deep copies

The following members are recommended for all types:
* `Str toS()` - convert the value to string. The base class `Object` and `TObject` provides a 
  default implementation for objects and actors. Values works without `toS`.


Global variables
----------------

Global variables are supported by the system. Global variables are always associated with a named
thread, which means that only the specified thread may access the variable. This is so that programs
using multiple global variables to represent a single global state will be less likely to be subject
to data races due to multiple threads accessing the variables. Disallowing access from all threads
but one makes sure programs provide accessors that enforce proper synchronization (due to them being
threaded).


Packages
---------

The type system is organized into packages, much like Java. Each package is represented by a
directory in the file system, except in the case of virtual packages. Virtual packages are created
for built in types in the case no directory is found in the file system.
