C++
====

Storm treats C++ (almost) like any other language. This part describes how C++ interacts with the
rest of Storm, even though C++ is not implemented in the Storm compiler.

Exporting functions
--------------------

The compiler itself is implemented in C++, and we therefore need a way to export C++ functions to
the type system. This is done by a preprocessor reading all header files and looking for specific
markers, namely `STORM_CLASS`, `STORM_VALUE`, `STORM_FN` and `STORM_CTOR` respectively. Classes
are declared like this:
```
class Foo : public Object {
    STORM_CLASS;
public:
    STORM_CTOR Foo();
    // ...
};
```
Values like this:
```
class Foo {
    STORM_VALUE;
public:
    // ...
};
```

Functions have their marker like this:
```
void STORM_FN foo();
```

Note that no functions are exposed unless marked. In many aspects, the storm runtime handles objects
and values like C++, so they can interact quite well as long as all types involved are properly
exposed to the compiler's type system. Do note, however, that Storm expects class- and actor types
to be passed and returned by pointer, and values by reference.

There is also an additional marker, `STORM_ASSIGN`, that marks a function as an assignment function
in Storm. For details on the available markers, see the file `Core/Storm.h`.

Note: If a function is not explicitly declared `virtual`, Storm assumes it is `final` even if the
function overrides a virtual function in a parent class. Storm understands the keyword `final` from
C++11, so if a function is marked both `virtual` and `final` it is considered `final` anyway.


Abstract classes
----------------

Abstract classes in C++ need special attention. In order to allow extending abstract classes from
languages in Storm, Storm needs to be able to instantiate abstract classes even though it would
normally not be allowed by C++. Therefore, this case needs special attention to work properly.

Abstract functions (or in C++ terms, pure virtual functions) need to be marked with `ABSTRACT` as
follows:

```
class MyInterface : public Object {
    STORM_ABSTRACT_CLASS;
public:
    STORM_CTOR MyInterface();
    void STORM_FN foo() ABSTRACT;
};
```

Also note that classes containing any abstract functions need to be marked with
`STORM_ABSTRACT_CLASS` rather than `STORM_CLASS`. The preprocessor will warn you if you have
abstract functions in a class marked `STORM_CLASS`, but currently not the other way around. Having
`STORM_ABSTRACT_CLASS` without abstract functions do not hurt, but it incurs a small overhead (more
work to do by the preprocessor, and sometimes slightly larger virtual function tables).


Exceptions
----------

All Exceptions in Storm inherit from the class `storm::Exception` and are therefore always
class-types. Aside from the abstract member function `message` that provides the exception message,
these classes work much like other class types in Storm. The notable exception is that all classes
inheriting from `storm::Exception` need to be declared as `STORM_EXCEPTION` instead of
`STORM_CLASS`. The preprocessor will inform you of this requirement. Furthermore, exception classes
may always be abstract, meaning that no special declaration is needed, as is the case with classes.

Exceptions are thrown and caught by pointer, and as they are class types they are allocated on the
GC heap. As long as this convention is followed, exceptions thrown in C++ can be caught in Storm,
and vice versa. If value types are used in Storm, you can also expect destructors to be called
when exceptions are thrown through Storm code, just as in C++.

Exceptions do not collect a stack trace by default, but provides a member `saveTrace` that
records a stack trace inside the exception that is shown later. This is done by many of the
exceptions used in Storm that are of fatal nature to aid debugging.


Garbage Collection
-------------------

By default, Storm uses the Memory Pool System for garbage collection. While this is mostly
transparent, the MPS imposes some restrictions on the type of data that can be stored in
heap-allocated objects. First and foremost, the MPS needs to know the layout of all objects on the
heap so that it is able to find and update all pointers in objects. This is handled by the
preprocessor for Storm as long as member variables are known by the preprocessor. Otherwise,
variables need to be annotated using the `UNKNOWN()` macro (see `Core/Storm.h` for
details). Furthermore, the MPS requires that all pointers in heap-allocated objects point to the
start of an allocation. This means that it is not possible to store a pointer (or reference) to an
element inside an array, or to a value stored inside another object.

Aside from this restriction, care also needs to be taken when passing pointers to GC allocated
objects to external libraries. As long as the pointer is not stored by the library anywhere other
than on the stack, everything is fine. If, however, the library stores a pointer inside memory
allocated using `malloc`, it is necessary to allocate this memory in a non-moving pool using
`runtime::allocStatic` and ensure the object is kept alive through other means. Another alternative
is to keep a pointer to the object in question on the stack, which means that the MPS will not move
the object. Be careful about compiler optimizations in this case, though!

On Linux, there is an additional thing to consider when writing low-level code. The MPS sends
signals to threads in the program in order to coordinate garbage collection. This means that it is
likely that some system calls (such as `read`, `write`, `sem_timedwait`, etc.) will fail with the
error code `EINTR`. It is therefore necessary to handle these failures if they can occur, since they
occur quite frequently.


Calling conventions
--------------------

The calling convention used in the compiler varies by platform, but it is generally `cdecl`, i.e.
the standard calling convention used in C. This is also used for member functions (the standard
here is usually `thiscall`) to unify the calling conventions used. However, they still differ
when dealing with return values that are not stored in registers. In the case of a non-member
function, a pointer to empty memory suitable for the result is passed as the first parameter
to the function. For member functions, the `this` pointer is always first, and the result
pointer is inserted as the second parameter.


Threads
--------

To switch between user level threads in C++, call the `os::UThread::leave()` function, or use any
synchronization primitive from `os::` that blocks the thread. Currently there is no way of doing the
same thing from another language. All of this is going through C++.

