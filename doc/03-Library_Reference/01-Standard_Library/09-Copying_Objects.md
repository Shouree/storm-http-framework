Copying Objects
===============

Since Storm needs to create copies of object graphs whenever messages are sent between threads, the
standard library contains mechanisms to achieve this. The functionality is implemented through the
function `clone` and the type `CloneEnv`.

The public interface to the object copying facilities is the `clone` function:

- `clone(T original)`

  Creates a deep copy of an object, `T`. `T` may be any type in the system. However, copyng an actor
  is considered to be a no-op.

  The returned object will be a copy of `original`, and the object will not refer to any objects
  (except actors) that were reachable from `original`. Furthermore, the returned object graph will
  have the same shape as that of `original`. That is, multiple references to the same object will be
  preserved, and cycles are properly handled.


The `clone` interface is implemented through the following facilities:

- `CloneEnv`

  This is a class that keeps track of the state during a clone (i.e. the environment in which the
  clone occurs). Technically, this means that the object keeps a map between objects in the old
  object graph to corresponding elements in the new object graph. Only the constructor is accessible
  from Storm due to the type-unsafe nature of the remainder of the interface.

- `clone(T original, CloneEnv env)`

  This function is intended to be called from `deepCopy` or similar functions. Behaves the same as
  `clone(T)`, but also accepts a `CloneEnv` object that contains the necessary state to handle
  shared objects and cyclic structures.

  This function first checks if `original` has already been cloned. If that is the case, then the
  already created clone is returned. Otherwise, `original` is copied using its copy constructor,
  followed by a call to `deepCopy`.


Furthermore, the `clone` system expects that all value and class types have the following members:

- *copy constructor*

  A constructor that accepts a reference to the object itself, and creates a shallow copy of the
  object. This member can simply just initialize itself using the default copy- or assignment
  behavior in the language that is being used.

- `deepCopy(CloneEnv env)`

  A member function that is expected to call `deepCopy(env)` member for any values in the type, and
  `clone(x, env)` for any classes in the type.


To illustrate how the system works, consider the following classes in Basic Storm, with the
accompanying implementation of copy constructors and `deepCopy` functions:

```bs
class MyClass {
    Nested nested;
    Value value;

    // Copy constructor:
    init(MyClass original) {
        init {
            nested = original.nested;
            data = original.data;
        }
    }

    // DeepCopy:
    void deepCopy(CloneEnv env) : override {
        nested = nested.clone(env);
        value.deepCopy(env);
    }
}

class Nested {
    Int x;

    init(Nested original) {
        init { x = original.x; }
    }
}

value Value {
    Int x;

    init(Value original) {
        init { x = original.x; }
    }
}
```

This allows `MyClass` to be copied by calling `clone(MyClass())`.

The system provides default implementations of both copy constructors and `deepCopy` as the types
`core.lang.TypeCopyCtor` and `core.lang.TypeDeepCopy`.
