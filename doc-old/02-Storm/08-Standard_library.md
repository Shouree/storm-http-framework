Standard Library
==================

Storm provides a standard library located in the `core` package. This section is to highlight the
most important types and functions from there, the rest can be explored from within Storm.

The most important types present in the standard library are the primitive types. In Storm,
primitive types are implemented to look as much as possible like user-defined types. However, they
follow different rules in calling conventions than a user-defined type of the same size. On X86,
using the `cdecl` calling convention in C++, user-defined objects are always returned on the stack,
while primitive types (pointers, integers, and so on) are always returned in the `eax`
register. Primitive types in Storm will of course follow these conventions to be compatible with C++
and to not lose execution speed.

Primitive types
----------------

Storm provides a few basic numeric types:

* Bool - boolean value.
* Byte - unsigned 8-bit integer.
* Int - signed 32-bit integer.
* Nat - unsigned 32-bit integer.
* Long - signed 64-bit integer.
* Word - unsigned 64-bit integer.
* Float - single-precision floating point number (32-bit).
* Double - double-precision floating point number (64-bit).

All types in storm have a fixed size across all platforms. Values may still be promoted during
calculations, so do not rely on overflow semantics. At the moment, the compiler does not optimize as
aggressively as current C compilers, so checks and similar things will not be optimized away.


Containers
-----------

Storm provides the following container types:

* `Str` - string, immutable.
* `Array<T>` - array.
* `Map<K, V>` - hash map.
* `Set<K>` - hash set.
* `WeakSet<K>` - weak hash set. Stores weak references to heap-allocated objects.
* `Queue<T>` - queue.
* `PQueue<T>` - priority queue implemented as a max-heap.
* `RefMap<K, V>` - hash map, hashes K by object identity always.
* `RefSet<K>` - hash set, hashes K by object identity always.

All containers except `PQueue<T>` and `Queue<T>` provide iterators that allow accessing individual
elements. These iterators work similarly to iterators in C++, except that they provide the members
`k` and `v` to access the key and value of the current element rather than using pointer
dereferencing. `k` is optional, and `Set<K>` does not provide that member. Due to its volatile
nature, `WeakSet<K>` uses another type of iterator, more similar to Java. This iterator is a
one-shot iterator that can be queried for the next element rather than an iterator to the begin and
the end. Both iterators are supported in the range-based for loop in Basic Storm.

The `Str` class stores characters in UTF-16 internally, but does not provide an API to access this
representation. Instead, iterators can be used to iterate through the string one UTF-32 codepoint at
a time. One can also use the function `core.io.readStr` to create a `TextReader` that allows reading
character by character from the string.

In Basic Storm, some of these have shorthands:
* `Array<T>` is `T[]`
* `Map<K, V>` is `K->V`
* `RefMap<K, V>` is `K&->V`

Iterators
----------

All containers except `PQueue<T>` and `Queue<T>` provide some way of iterating through the
individual elements in the container. Storm provides two interfaces for this: C++-style iterators
and ranges.

An iterator is, just like in C++, a pointer to a position in a container. Iterators can be advanced
to the next position using the `++`-operator and compared to other iterators using `==` and
`!=`. Thus, to represent a range of elements, one iterator to each end is required. Therefore,
containers using this interface provide two operations: `begin` and `end` to acquire the entire
range contained in the iterator. Ranges are exclusive, meaning that the iterator referred to by the
`end` operation refers to one element after the last element, and not a valid element. Iterators may
be advanced to the `end` element and compared to the `end` iterator, but not further. The values
referred to by the iterator are retrieved by the `k` and `v` operations (for key and value,
respectively). Not all containers, for example the `Set`, provide the `k` operation on their
iterators.

Other containers, most notably `WeakSet<T>` uses another kind of iterator, similar to what is found
in Java. We will refer to these as *range iterators*. A range, just like an iterator, represents a
position in a container, but keeps track of the end of the range itself. A range iterator provides a
single operation, `next`, that advances the iterator and returns the current element, or `null`
whenever the end of the range is reached. The return value of `next` is therefore always `Maybe<T>`.


Other types
------------

Storm also provides some higher-level types:

* Str - string, immutable.
* StrBuf - string buffer for more efficient string concatenations.
* Thread - represents a OS thread.
* Future<T> - future, for inter-thread communication.
* FnPtr<R, ...> - function pointer.
* Moment - timestamp from a high-resolution timer.
* Duration - difference between two `Moment`s.
