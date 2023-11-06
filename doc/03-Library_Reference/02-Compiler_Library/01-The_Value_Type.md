The Value Type
==============

The class [stormname:core.lang.Value] represents a value of some type that is stored inside a
variable. As such, a `Value` is essentially just a reference to a `Type` in the name tree (or
`void`, if the reference is `null`). It does, however, also contain the variable `ref` that
indicates if the stored value is a reference to the value or not. This is used to distinguish
reference and non-reference parameters in parameter lists for example. Note that `ref` is to be
interpreted in addition to the usual semantics of the referred type. That is, if the `Value` refers
to a class or actor type, and has its `ref` set to true, then the variable is to be considered a
reference to the reference to the actual instance.

Apart from packing these two values together, the `Value` class contains a large number of
convenience functions for checking properties of types. It is therefore not uncommon to cast types
to a `Value` before comparing them:

```stormdoc
@core.lang.Value
- .type
- .ref
- .asRef(*)
- .mayReferTo(Value)
- .mayStore(Value)
- .matches(*)
- .size()
- .desc()
- .copyCtor()
- .destructor()
```

There are also a number of utility functions available:

```stormdoc
@core.lang
- .thisPtr(Type)
- .common(Value, Value)
```


Apart from the above functions for comparing types, the `Value` class also provides information
related to code generation. 

```stormdoc
@core.lang.Value
- .returnInReg
- .isValue
- .isObject
- .isClass
- .isActor
- .isPrimitive
- .isAsmType
- .desc()
- .copyCtor()
- .destructor()
```
