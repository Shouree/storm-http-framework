Objects in C++
==============

Since Storm uses a garbage collector to manage memory, some care needs to be taken when working with
objects in C++. Furthermore, Storm is designed to be useable as an embedded language. Because of
this, the entire Storm system is represented as an instance of the `storm::Engine` class. Different
engines are designed to be able to operate separately. Therefore, all heap-allocated objects are
associated with an `Engine` instance, since they are allocated on a heap associated with a
particular `Engine`.

Both of these things makes object allocation and management needs some additional care:

- The garbage collector needs to be able to scan and update all pointers in all heap-allocated
  objects. This means that all pointers need to be visible to the garbage collector. In most cases,
  this is handled by the preprocessor, but some care needs to be taken when working with `void`
  pointers, for example. Another restriction is that all pointers to heap-allocated objects must
  refer to the start of the allocation. It is therefore not possible to encode additional
  information in the unused bits of a pointer, or to store pointers to the middle of objects (such
  pointers may exist on the stack, however).

- Destructors are not called (not even the default destructor) if it is not declared in the class,
  or if any contained values have destructors. This is typically not a problem, but if the class
  contains types unknown to Storm, where the destructor performs some important operation, it might
  need to be declared explicitly. Finally, it is worth noting that the destructor is not called
  immediately after an object becomes unreachable. There is a, potentially unbounded, delay before
  objects are destroyed.

- Objects need to be allocated using the *placement new* syntax in order to specify which `Engine`
  instance the object belongs to. An `Engine` represents an instance of the entire Storm language.
  It is thus possible to create multiple Storm "worlds" in the same process.

  Objects are thus allocated by writing:

  ```
  new (<engine>) T(<parameters>);
  ```

  Where `<engine>` is an expression that evaluates to either an `Engine`, or a heap-allocated
  object. All heap-allocated objects have the member function `engine()` that retrieves the `Engine`
  they are associated with. So, for code running in member functions of heap-allocated objects, the
  objects can typically be allocated as:

  ```
  new (this) T(<parameters>);
  ```

  In free functions, an engine can be passed if the first parameter is declared as `EnginePtr`. This
  type has a member `v` that is a reference to an `Engine`.
