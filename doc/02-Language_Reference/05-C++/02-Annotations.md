Annotations in C++
==================

Since types and functions exposed to Storm need to follow the expectations of the Storm type system,
functions that should be visible to Storm need to be explicitly exported. This is done by annotating
them using a suitable annotation.


Considerations
--------------

Functions and types that are exposed to Storm need to follow the following conventions:

- Classes or actors may only contain pointers to the start of other GC-allocated objects. All
  pointers to GC-allocated types need to be known to Storm.

- All parameters and return values of functions need to be visible to Storm.

- If a function returns- or accepts a `Maybe`-type for a class or an actor, they should be expressed
  as `MAYBE(T *)`. Otherwise, the `Maybe<T>` type can be used (with some limitations).


Functions
---------

Functions, both member functions and non-member functions, are annotated using the `STORM_FN`
annotation. It is placed between the return type and the function name, as follows:

```
Str *STORM_FN stringFunction(Str *param);
```

If a function requires a reference to the current `Engine` (e.g. for memory allocations), then the
system will automatically pass an `Engine` if the first parameter is declared as `EnginePtr`:

```
Str *STORM_FN toString(EnginePtr e, Int param);
```

The function above will appear as a function accepting a single parameter.

For assignment functions, the `STORM_ASSIGN` can be used instead.

Functions can also be associated with threads using `ON(<thread>)` as follows:

```
Str *STORM_FN threadFunction(Str *param) ON(Compiler);
```

This can also be done for member functions in values.


Types
-----

Values are annotated with the `STORM_VALUE` annotation like below:

```
class MyValue {
    STORM_VALUE;
public:
    // ...
};
```

Classes and actors are annotated using `STORM_CLASS`, similarly to the value. Objects and classes
do, however, need to inherit from `storm::OBject` or `storm::TObject`. This can be done either
directly or indirectly. So for a class, one can write:

```
class MyClass : public Object {
    STORM_CLASS;
public:
    // ...
};
```

An similarly, for actors:

```
class MyActor : public TObject {
    STORM_CLASS;
public:
    // ...
};
```

Actors tied to a particular object can be created by inheriting from the class `ObjectOn<T>`. For
example:

```
class MyActor : public ObjectOn<Compiler> {
    STORM_CLASS;
public:
    // ...
};
```

As with functions, constructors are not exposed automatically. They need to be marked with
`STORM_CTOR` or `STORM_CAST_CTOR`. The exception is copy-constructors, which are exported
automatically.

Member functions can be declared as abstract by appending `ABSTRACT` where `= 0` would appear in
C++. Classes with abstract members need to be defined using `STORM_ABSTRACT_CLASS` instead of
`STORM_CLASS`.

Finally, classes inheriting from exceptions need special care. They need to be declared as follows:

```
class EXCEPTION_EXPORT MyException : public storm::Exception {
    STORM_EXCEPTION;
public:
    // ...
};
```

Member Variables
----------------

Member variables do not typically need to be annotated. The system generates metadata for all
members by default in order to be able to generate metadata for garbage collection. However, if an
object contains members that the preprocessor does not know about, it might be necessary to annotate
them using the `UNKNOWN` macro. This macro informs the preprocessor how the member is used. The
following modes are available:

- `UNKNOWN(INT)` - The variable following is an integer variable (size equal to `Int` in Storm).
- `UNKNOWN(PTR_NOGC)` - The variable following is pointer-sized, but does not refer to any data on
  the GC heap. As such, it is not scanned during garbage collections. Pointers to data on the
  C++ heap is fine to store in this type of variables.
- `UNKNOWN(PTR_GC)` - The variable following is pointer-sized, and points to data on the GC heap.
  As such, it must point to the start of a valid GC allocation, to somewhere on the C++ heap, or
  to null. In particular, it may *not* refer to anywhere but the start of GC allocations.


Threads
-------

Threads are declared using the `STORM_THREAD` macro, like below:

```
STORM_THREAD(MyThread);
```

As with many other things in C++, the thread also needs to be defined in a `.cpp`-file. This can be
done in a number of ways depending on the integration required. The first, `STORM_DEFINE_THREAD` is
simply used when nothing special is required:

```
STORM_DEFINE_THREAD(MyThread);
```

If the library wishes to create the thread manually (e.g. to provide custom logic for integrating
with the scheduler), it can be declared as follows:

```
STORM_DEFINE_THREAD_WAIT(MyThread, <ptr>);
```

Where `<ptr>` is a pointer to a function that accepts a reference to an `Engine`, and returns an
`os::Thread`. This function will be called when the thread needs to be created.
