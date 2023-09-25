Type System
===========

The type system in Storm is inspired from languages like C++ and Java, but has been extended to
allow associating data with threads in order to allow safe multithreading. The type system in Storm
is, to an extent, compatible with C++, and it is possible to seamlessly call functions in C++ from
Storm, and to extend classes defined in C++ in Storm.

There are three different kind of types in Storm. Each of them have different behavior with respect
to parameter passing and threading:

- **Values**

  Values represent small types that should be treated as a unit. A good example is a 2D coordinate
  that consists of an x coordinate and a y coordinate.

  Values have by-value semantics. That is, assignments make a copy, and a copy is passed to and from
  functions. As such, they are always stored inline in the surrounding context. That is, a value
  declared as a member of another type is stored in the same allocation as the other members, and a
  value declared as a local variable is stored on the stack.

- **Classes**

  Classes represent larger types that are more expensive to copy, and types where the exact type
  might not be known. Because of this, class types are always allocated on the heap, and are managed
  by reference. That is, assignments only make a copy of the reference to the class. Similarly, when
  passed to or from functions, classes are passed by reference. This makes it possible to store a
  reference to a derived type in a variable that has the type of a base class, and to override
  member functions with virtual dispatch. All class types inherit, either directly or indirectly,
  from the `Object` class.

- **Actors**

  Actors are similar to classes in that they are managed by reference. In contrast to classes they
  are associated with an OS thread, and any members of the class may only be accessed by the
  associated OS thread. All actors inherit from the `TObject` type (for Thread Object). Note that
  `TObject` and `Object` are unrelated in the Storm type system.


Classes and actors might initially appear similar. The difference between the two lies in how they
interact with the [threading model](md://./Threading_Model). Since different OS threads are not
allowed to share data, classes need to be copied whenever a message from one thread to another is
posted. This is not the case with actors, since actors require that their members are accessed from
the proper OS thread (i.e. sending messages if not accessed by the proper OS thread already).

As such, the three kinds of types form a progression from pure by-value semantics (values) to pure
by-reference semantics (actors). Classes is between the two, and behaves like values when messages
are involved (i.e. it is not possible to share them between different OS threads). They do, however,
behave like references within a single OS thread. Because of this, class types are generally
designed to behave more like values compared to languages like Java. For example, when comparing
classes for equivalence using the `==` operator, most languages call the overloaded `==` operator to
check for equality, and not whether the two references refer to the same object or not.

This means that we can view the semantics from two perspectives. When we are working within a single
OS thread, by-value semantics is achieved using values, and by-reference semantics using classes (or
actors). When working between threads, by-value semantics is achieved using values or classes, and
by-reference semantics using actors.

The semantics are summarized in the table below:

```inlinehtml
<table>
    <tr>
        <td></td><th>Values</th><th>Classes</th><th>Actors</th>
    </tr>
    <tr>
        <td>Assignment</td><td>Copy</td><td>By reference</td><td>By reference</td>
    </tr>
    <tr>
        <td>Function calls</td><td>Copy</td><td>By reference</td><td>By reference</td>
    </tr>
    <tr>
        <td>Messages</td><td>Copy</td><td>Copy</td><td>By reference</td>
    </tr>
</table>
```


Virtual Functions
------------------

Storm supports virtual functions for classes and actors. When a function in a sub-class overrides a
function in a base class, Storm utilizes vtable-based virtual dispatch automatically. This will only
be done for functions that are overridden, so no additional costs are incurred unless the feature is
used.

A function in a derived class does not have to match a function in a parent class exactly. It is
possible to accept less specific types in the derived function. However, this means that a single
function could override two functions in a parent class. This is not allowed. This happens if, for
example, the parent class contains the functions `add(Str)` and `add(Url)`, and the derived class
contains the function `add(Object)`. However, if the parent class would also contain an exact match
(`add(Object)` in this case), the exact match is preferred over inexact matches. This restrictive
behavior aims to reduce unintentional surprises in the overriding behavior.


Functions, Types, and Threads
-----------------------------

For each type and each function in the system, the type system keeps track of which threads are
allowed to use the function or type. There are three different options:

1. Any thread

   This is the default behavior for values, classes, and free functions. It means that all threads
   in the system may call the function, and that the type may be used by all threads. It is,
   however, worth noting that the constraint that different OS threads may not share data still
   applies. This simply means that data accessible from any thread might need to be copied to ensure
   that it is not shared.

2. A named thread

   Both free functions and actors may be associated with a statically known, named thread (using the
   `on` keyword in Basic Storm). For types, this means that the type is an actor type, and its
   members can only be accessed from the specified OS thread. For functions, this means that the
   function may only be called from the specified OS thread.

   To ensure correct usage of these functions and types, the compiler inserts code to send a message
   to the declared thread whenever it is not possible to statically determine that it is unnecessary
   to send a message. For example, consider a function `f` that is associated with thread `T`,
   either by being a member of an actor, or a free function explicitly associated with thread `T`.
   Any calls from other functions associated with thread `T` will always call `f` directly. However,
   functions associated with other named threads, any threads, or a dynamic thread will always send
   a message.

3. Dynamic

   Actors might additionally be associated with a thread that is known only at runtime (in Basic
   Storm: declared as `on ?`). This means that the actor is passed a `Thread` at creation, and is
   associated to that thread throughout its lifetime. Similarly to actors associated with named
   threads, this means that only the associated thread may access the members of the actor. However,
   since it is not generally possible to determine which dynamic actors are associated with the same
   thread, messages are sent for all accesses to such actors. The only exception is when a member
   function accesses another member of the same instance.



Deep Copy
---------

Since the threading model requires the ability to copy values and classes whenever they are sent as
a message, Storm needs to be able to copy classes and values.

There are two types of copy operations: shallow copies and deep copies. A shallow copy can be
created, just like in C++, by calling the copy constructor of the type. In Basic Storm, a copy
constructor is automatically generated if you do not provide one yourself.

Deep copying makes a copy of an entire object graph. This copy supports copying arbitrary object
graphs, including graphs with cycles and shared objects. It is implemented using the
compiler-generated `clone` function in the package `core`. The function performs the following
operations (using Basic Storm-like syntax):

```
T clone(T v) {
    T copy(v);
    CloneEnv env;
    copy.deepCopy(env);
    return copy;
}
```

It first calls the copy-constructor, then creates a built-in object called `CloneEnv` that keeps
track of the state when traversing the object graph. Finally it calls the `deepCopy` member to
perform the deep copy and returns the copy. `deepCopy` is also automatically generated by Basic
Storm, and this function makes sure to either clone or call `deepCopy` on any member variables in
that type.

The `deepCopy` functions are implemented like this:
```
void deepCopy(CloneEnv env) {
    super.deepCopy(env);
    a = clone(a, env);
    // ...
}
```

Note that cloning an actor simply returns the actor itself, since actors are not copied when passed
as a message.
