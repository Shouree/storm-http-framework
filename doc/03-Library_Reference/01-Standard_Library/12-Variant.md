Variant
=======

The standard library contains the type [stormname:core.Variant] that represents an object of any
type in the system. Due to the need to accommodate any type, it has to allocate values on the heap
rather than internally. This means that using a `Variant` incurs a performance penalty over
situations where the types are known statically.

The `Variant` has no facility to extract the current value in a safe manner. It is necessary to use
[weak casts](md:/Language_Reference/Basic_Storm/Code/Type_Conversions) to extract the contained
type.

The `Variant` contains constructors that automatically cast any types into a `Variant`
implicitly. This allows functions that accept `Variant`s to be used conveniently.

Apart from the constructors, the `Variant` has the following members:

```stormdoc
@core.Variant
- .__init()
- .empty()
- .any()
- .has(*)
- .type()
```

It is also possible to print variants using `<<` or `toS` as usual.
