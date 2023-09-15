Iterators
===========

Iterators in Storm's standard library are fairly similar to iterators in C++.

An iterator is a pointer into some position of a container. To get iterators to a container, call
that container's `begin` and `end` function. The `begin` iterator points to the first element in the
container, while the `end` iterator points one element past the last element in the container.

When an iterator has been created, the iterator assumes the referred container is not changed during
the iterator's lifetime. Modifying the container (other than altering any contained elements) make
the associated iterators behave in an undefined way.

Iterators do not inherit from a base class, they only have to support the following operations:

* `++i`: advance the iterator one step. (`i++` is also good to support).
* `i == j`: check iterators for equality. Iterators pointing to the last element of a container always compare equal, regardless of which container the iterator was pointing into originally.
* `i != j`: inequality check.
* `v`: get the value currently pointed to. Throws an exception if the iterator is invalid.
* `k`: if the container associates a key to each value, this member is used to get the key. This member may be left out.

As can be seen, iterators to containers like `Map` work differently in Storm compared to C++. In
Storm, the iterator allows you to access the key and the value separately, rather than returning a
pair with both the key and the value. The reason for this is that Storm has no natural *dereference*
operation, and therefore `k` and `v` are easier to read and write. This also allows containers to
provide optional keys for their values. For example, `Array` provides the index of the current
element as the key, which can be used if desired. Basic Storm implements a range-based for loop that
uses this.

A second type of iterators are provided by the `WeakSet<T>` class. This type has a single operation:

* `T? next()`, which returns the next element if it is available, or `null`.

This type is also supported by the for loop in storm. This type does, however, not support keys.
