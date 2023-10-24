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

- [stormname:core.unsafe.RawPtr.asVariant]

  Extracts the contents of the raw pointer as a `Variant`.


The raw pointer type also contains types for reading and writing to the pointer. These are named
`readX` and `writeX`, where `X` is one of the types `Bool`, `Byte`, `Int`, `Nat`, `Long`, `Word`,
`Float`, `Double`, or `Ptr`. They read or write the corresponding type to an offset in the pointed
object. The suffix `Ptr` reads or writes a `RawPtr` object as a pointer.

All read functions have the signature: `X readX(Nat offset)`, and the write functions have the
signature `void writeX(Nat offset, X value)`. The `offset` is the number of bytes into the object
that should be read. Note that the offset typically has to be aligned to the size of the type,
otherwise the read operation might fail on some architectures.

If the pointer refers to an array (in particular, values allocated on the heap are stored as arrays
of size 1), the offset is relative to the start of the contents of the array, and not the start of
the array allocation. This means that it is not possible to overwrite the array header using the
`read` and `write` operations. Instead, the pointer provides the members `readCount` and
`readFilled` to access the corresponding fields in the array header. It is possible to update the
`filled` field by calling the `writeFilled` member, but the `count` field is static. As a
convenience, these functions work if the pointer points to an object as well. In that case they act
as if the pointer points to an array of size 1 that is filled. Finally, the function `readSize` can
be used to compute the total size of a single element in the allocation. As such, the maximum offset
that is usable is `p.readCount * p.readSize`.


Pinned Set
----------

The pinned set is a set of arbitrary pointers to memory. In contrast to most other pointers in the
system, these pointers may refer anywhere in memory. This includes to the middle of objects, and to
values on the stack. Because of this, it is only possible to query the pinned set about whether a
particular address is contained within or not. It is not possible to retrieve the pointers, as they
may be stale and refer to nothing.

The pinned set is scanned by the garbage collector as if it was a call stack. This means that the
garbage collector treats everything in the pinned set as a *root*, which in turn means that objects
inserted into the pinned set will be kept alive until the pinned set is cleared or destroyed. Since
the pinned set is a root, it is possible to accidentally create a series of references from an
object in the pinned set back to the pinned set itself. This causes the pinned set to never be
freed. To overcome this, it is important to clear the contents of the pinned set when it is no
longer needed.

The pinned set has the following members:

- [stormname:core.unsafe.PinnedSet.__init()]

  Create an empty pinned set.

- [stormname:core.unsafe.PinnedSet.put(core.unsafe.RawPtr)]

  Add the pointer to the set of pointers.

- [stormname:core.unsafe.PinnedSet.put(core.lang.unknown.PTR_NOGC)]

  Add the pointer to the set of pointers. Note that `PTR_NOGC` can refer to anything in the system,
  while `RawPtr` is limited but easier to use.

- [stormname:core.unsafe.PinnedSet.has(core.unsafe.RawPtr)]

  Check if the set contains a pointer into any point of the memory pointed to by the supplied
  `RawPtr`.

- [stormname:core.unsafe.PinnedSet.has(core.unsafe.RawPtr, Nat)]

  Check if the set contains a pointer to offset (`Nat`) into the pointer.

- [stormname:core.unsafe.PinnedSet.has(core.unsafe.RawPtr, Nat, Nat)]

  Check if the set contains a pointer to the area between the offset (the first `Nat`), and a
  further size (the second `Nat`) position of the supplied pointer. That is, is there a pointer that
  lies between `pointer + offset` and `pointer + offset + size` (ignoring any array headers).

- [stormname:core.unsafe.PinnedSet.offsets(core.unsafe.RawPtr)]

  Return all offsets into the object pointed to by the pointer that are in the pinned set as an array.
