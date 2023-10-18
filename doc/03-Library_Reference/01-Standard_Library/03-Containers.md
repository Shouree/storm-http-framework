Containers
==========

The standard library provides a number of containers, that can store collections of other elements.
They are implemented as parameterized types, meaning that the system generates concrete types for
different types. These types are not immediately related to each other. That means that the type
`Array<Base>` is not convertible to `Array<Derived>` easily. Due to the implementation of the types
in C++, most containers are, however, derived from a common base class. For example, both
`Array<Str>` and `Array<Int>` inherit from `ArrayBase`. The base classes are generally not very
interesting, as they only implement the part of the interface that is the same regardless of the
type stored in the container.

In the remainder of this document, the following notation is used for generic types:

- `T` - any type
- `H` - a type that has a `hash` and a `==` member, or an actor (actors are always hashed based on their identity)
- `C` - a type that can be compared using `<`
- `O` - an object or an actor
- `A` - an actor


All containers in the standard library have the following members:

- `count()` - get the number of elements in the container
- `empty()` - return `true` if the container is empty
- `any()` - return `true` if the container contains at least one element
- `reserve()` - reserve space for future insertions to avoid reallocations
- `clear()` - clear the contents of the container, making it empty
- Iterators - an interface for iterating the container (except `PQueue`)

The types below are generally usable in C++ as well, but there are some minor differences in the
interfaces due to how the integration between C++ and Storm works.

## Array

The type `Array<T>` (`T[]` in Basic Storm) is an array of the type `T`. Elements can thus be
accessed by index using the `[]` operator. Dynamically grows to fit the elements, but does not
automatically shrink.

It has the following members:

- `[]` - member access by index (indices are `Nat`), throws on out of bounds
- `push(T)` or `<<` - push an element at the end of the array
- `pop()` - remove an element from the end
- `insert(Nat, T)` - insert an element at the specified position
- `append(Array<T>)` - append contents form another array
- `first()` - get the first element, throws if none exist
- `last()` - get the last element, throws if none exist
- `sort()` - sort the array, optionally provide a comparison function
- `sorted()` - sort and return a copy of the array
- `random()` - get a random element

## Queue

The type `Queue<T>` is queue of elements of the type `T`. Dynamically grows to fit the elements, but
does not automatically shrink.

It has the following members:

- `push(T)` - add an element to the queue
- `pop()` - remove an element from the queue
- `top()` - get the top element of the queue, throws if none exist


## Priority Queue

The type `PQueue<C>` is a priority queue of elements of type `C`. Uses the `<` operator of the type
to compare elements and extract the largest one. It is also possible to create a `PQueue` with a
custom predicate function that is used instead of the default `<` operator, or for types without a
`<` operator. It is implemented as a heap internally, and returns the largest element first.

It is not possible to iterate through elements in the priority queue.

It has the following members:

- `push(C)` - add an element to the priority queue
- `pop()` - remove the element with the highest priority
- `top()` - get the element with the highest priority


## Hash Map

The type `Map<H, T>` (`H->T` in Basic Storm) is a hash table with keys of type `H` and values of
type `T`. As such, elements are not ordered in any particular way when iterating through the
container. Automatically grows to fit the elements, but does not shrink automatically unless
cleared.

Elements can be accessed in any of the following ways:

- `put(H, T)` - insert an element, or update an existing one
- `has(H)` - check if an element is present
- `get(H)` - get an element from the hash table, throws an exception if the element does not exist
- `get(H, T)` - get an element from the hash table, return `T` if it does not exist
- `at(H)` - get an element, returns `Maybe<T>` that is `null` if the element was not found
- `find(H)` - get an iterator to an element
- `[]` - get an element, insert a default-constructed element and return that if it does not exist,
  requires that `T` has a default constructor
- `remove(H)` - remove an element from the hash table, returns whether it existed or not

There is also a type `RefMap<O, T>` (`O&->T` in Basic Storm) that is identical to `Map<H, T>` except
that it always uses object identity to identify objects. The `RefMap` type can thus not use value
types as keys.

## Hash Set

The type `Set<H>` is a hash table with keys of type `H`. The interface is similar to `Map`, but the
functionaliy is reduced since there are no values associated with the keys. As such, the remaining
ones are:

- `put(H)` - insert an element, or update an existing one
- `put(Set<H>)` - insert all entries from another set into this one
- `has(H)` - contains an element?
- `get(H)` - get the inserted element, throws if not present
- `at(H)` - get an element, returns `Maybe<H>` that is `null` if the element was not found
- `[]` - get an element, inserts and returns `k` if not present

There is also a type `RefSet<O>` that is identical to `Map<H, T>` except that it always uses object
identity to identify objects. It can thus not use value types as keys.


## Weak Hash Set

The type `WeakSet<A>` is a weak set of actors. The actors are compared based on their identity.
Since the set is weak, elements that are added to the set are not kept alive unless other parts of
the system refer to the actors. As such, a `WeakSet` is useful to keep track of registered
callbacks, but to have them automatically de-registered whenever they are no longer used elsewhere.
Note, however, that finalization and removal from a `WeakSet` is not guaranteed to happen quickly,
and objects may thus linger for a while after all non-weak references have disappeared.

In contrast to most other containers, the `WeakSet` may automatically shrink its storage when
enough objects have been removed.

The interface to `WeakSet` is fairly simple:

- `put(A)` - insert an element
- `has(A)` - check if an element exists
- `remove(A)` - remove an element
