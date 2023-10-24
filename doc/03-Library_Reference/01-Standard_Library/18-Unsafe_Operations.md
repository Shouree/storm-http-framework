Unsafe Operations
=================

The package `core.unsafe` contains a collection of primitives that allows inspection and
manipulation of memory in unsafe ways. It currently contains the type [stormname:core.unsafe.RawPtr]
and [stormname:core.unsafe.PinnedSet].

Raw Pointers
------------

The type [stormname:core.unsafe.RawPtr] is a raw pointer. Compared to other types in the system, the
raw pointer interface allows arbitrary reads and writes relative to the pointer. The system still
places some restrictions on what a raw pointer may refer to in order to ensure that garbage
collection can proceed normally. In particular, a raw pointer needs to refer to the start of an
object, and it must refer to a heap-allocated object.

Raw pointers can be created from any class or actor in the system. It is also possible to create a
raw pointer from a `Variant`. Since `Variants` heap-allocate values (as if they were arrays), this
allows a raw pointer to point to values as well. The member `copyVariant` is used for this task.

Raw pointers have the operations one would expect in terms of comparison, copying, assignment and to
string. It also has the operators `empty` and `any` to allow convenient checks for null. It also has
the following operations for inspecting and extracting the contained pointer:

- [stormname:core.unsafe.RawPtr.type]

  Acquire the type of the object. May return `null` if no type information is available.

- [stormname:core.unsafe.RawPtr.isValue]

  Check if the pointer refers to an array of values.

- [stormname:core.unsafe.RawPtr.asObject]

  Extracts the pointer as an object.

- [stormname:core.unsafe.RawPtr.asTObject]

  Extracts the pointer as an actor.

