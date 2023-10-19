Iterators
=========

There are two types of iterators present in Storm's standard library: *pointer iterators* and *lazy
iterators*. Both are natively supported in the Basic Storm
[for-loop](md:/Language_Reference/Basic_Storm/Code/Loops), along with index-based iteration.

Pointer Iterators
-----------------

The first type of iterators, *pointer iterators*, are similar to traditional iterators in C++. A
pointer iterator is an abstraction of a pointer to an element in a container. The iterator knows how
to access the current element, and to advance through the data structure. There is, however, no
built-in notion of the end of the iterated range, so it is necessary to keep track of the end
manually. While this puts additional burden on the programmer, it means that it is possible to
use two iterators to provide a range on which an algorithm should operate on, for example.

Pointer iterators are retrieved from a container by calling the `begin` and `end` members. They
return an iterator to the beginning and the end of the container respectively. The `end` iterator
points to a fictive element that is located after all elements in the container. It is not possible
to access the value at this position.

The returned iterators contain the following members:

- *copy constructor* and *assignment operator*

  Allows copying and assigning iterators. This allows making copies of iterators to re-start
  iteration at a later time, for example.

- `++*` and `*++`

  Advances the iterator one step, either using prefix or postfix notation. As expected by the prefix
  and postfix increment operators, they return the iterator either as it was before it was
  incremented, or the result after incrementation. Note that this typically means creating a copy of
  the iterator for postfix increment, which is not necessary for prefix increment.

  Incrementing an iterator referring to the end of a container is generally safe, and simply leaves
  the iterator unmodified.

- `+`

  Advances the iterator a fixed number of steps (indicated by a `Nat`). This operator is optional
  and not supported by all iterators. The effect is that `it + n` is equivalent to executing `++it`
  *n* times, but usually in a more efficient manner.

- `==` and `!=`

  Compares two iterators for equality or inequality. This allows comparing

- `v()`

  Retrieve the value at the current location. If the iterator is the end iterator, then this
  operation throws an error.

- `k()`

  Retrieves the key of the current location. If the iterator is an end iterator, then this operation
  throws an error. This member is only present in iterators where the concept of *key* is relevant.
  For example, it is provided in iterators for `Array` and `Map`, but not for `Set`.

These iterators can be used to iterate through a hash map as follows:

```bsstmt
Map<Int, Str> map;

for (Map<Int, Str> i = map.begin(); i != map.end(); ++i) {
    print("Key ${i.k} -> value ${i.v}");
}
```

Or, of course, using the for-each syntax:

```bsstmt
Map<Int, Str> map;
for (k, v in map) {
    print("Key ${i.k} -> value ${i.v}");
}
```

Lazy Iterators
--------------

Lazy iterators provide a different interface that is a bit less flexible than the pointer iterators.
It was originally designed to meet the constraints required for the `WeakSet` container, where it is
generally not possible to know the exact contents of the iterated range before actually traversing
the elements. This idea has also been proved useful in other places, for example in the [database
library](md:/Library_Reference/SQL_Library).

A lazy iterator is retrieved from a container using the `iter()` member. This creates a new lazy
iterator that initially refers to the start of the range. In contrast to pointer iterators, the
iterator knows both the current element, and when the end of the range has been reached. As such,
the concept of an end iterator is not necessary for a lazy iterator. Furthermore, it is only
possible to retrieve each element once, which means that the iterator may produce elements lazily as
they are requested. For this reason, a lazy iterator does not support the concept of keys.

A lazy iterator has the following interface:

- *copy constructor* and *assignment operator*

  A lazy iterator can be copied. Depending on what the iterator retrieves data from, this may or may
  not create iterators that are independent of each other. For example, iterators from `WeakSet` can
  be copied, and will retain their previous position (modulo objects that have been destroyed),
  while iterators from the SQL library are generally linked together since they retrieve the next
  row from a database on demand.

- `next()`

  Retrieves the next element, and advances the iterator to the next element. This member returns a
  `Maybe<T>` that contains the retrieved element, or `null` if no mor elements were available.


As such, a lazy iterator can be used as follows in Basic Storm:

```bsstmt
WeakSet<Named> set;

WeakSet<Named> i = set.iter();
while (elem = i.next) {
    print("Element ${elem}");
}
```

Again, the for-each syntax can also be used:

```bsstmt
WeakSet<Named> set;

for (elem in set) {
    print("Element ${elem}");
}
```
