Objects and Actors
==================

The standard library contains the two types `core.Object` and `core.TObject`. These are the types
that all other class types and actor types in the system inherit from. That is, all class types
inherit from `core.Object`, and all actor types inherit from `core.TObject`.

Object
------

The type [stormname:core.Object] contains the following members:

- [stormname:core.Object.toS()]

  Create a string representation of the object. The default implementation simply calls
  `toS(StrBuf)` to create the string representation.

- [stormname:core.Object.toS(core.StrBuf)]

  Create a string representation, and write it to the provided [string buffer](md:Strings). The
  default implementation outputs the type of the object and its current address. Note that the
  address may change during the lifetime of the object, so it is not a stable identifier.

  Many types in the system provide some form of string representation to aid debugging. The
  preferred method of providing a string representation is to override this version of `toS`.

- [stormname:core.Object.deepCopy(core.CloneEnv)]

  Used to provide [deep copies](md:Copying_Objects) of object.


TObject
-------

The type [stormname:core.TObject] is the base class of all actors (`TObject` stands for "Thread
Object"). It has the same interface as `core.Object`, except that `deepCopy` does not exist (actors
typically don't need to be copied), and it instead contains the member `associatedThread` that
specifies which thread the actor is associated with. The associated thread is set through a
parameter to the constructor, and it is not possible to change the associated thread after creation.


Utilities
---------

The standard library also provides the following function for comparing the types of classes. It is
useful to compare types for equivalence in a `==` operator, for example.

```stormdoc
- core.sameType(*)
```
