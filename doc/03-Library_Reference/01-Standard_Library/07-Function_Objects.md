Function Objects
================

The type `core.Fn<R, ...>` represents a pointer to a function, optionally with a bound first
parameter. Basic Storm has [custom syntax](md:/Language_Reference/Basic_Storm/Names) for this type.

The parameters to the type dictate the signature of the function to be called. The first parameter
is the return value of the function (which may be `void`). Any remaining parameters are the types of
the formal parameters to the function. These may *not* be `void`. Below are a few examples to
illustrate the idea:

1. `core.Fn<void>` denotes a pointer to a function with zero parameters that returns nothing.
2. `core.Fn<core.Str, core.Nat>` denotes a pointer to a function that accepts a `core.Nat` and returns a `core.Str`.
3. `core.Fn<void, core.Nat, core.Int>` denotes a pointer to a function that accepts a `core.Nat` and a `core.Int` and returns nothing.

The member `call` is used to invoke the function pointed to. The signature of the `call` member thus
corresponds to the parameters to the type. In the examples above, the call has the following
signatures (the `this` parameter is omitted for clarity):

1. `void call()`
2. `core.Str call(core.Nat)`
3. `void call(core.Nat, core.Int)`


The `Fn` type may also contain a pointer to a class or an actor that should be used as the first
parameter of the call. Because of this, the signature of the class that is pointed to differ from
the signature of the `call` function. For example, the function pointer in case 1 above may call a
function that accepts a `Str` as a formal parameter, but the `Str` is stored inside the `Fn` object
itself.

Even though the `Fn` type is limited to binding classes or actors as the first parameter, this is
enough to implement function objects with arbitrarily complex state. This is, for example, done to
implement [lambda functions](md:/Language_Reference/Basic_Storm/Code/Function_Objects) in Basic
Storm.

The `Fn` type does not have a constructor. Rather, it needs to be created by the language in use, or
from the member `pointer` in `core.lang.Function`.
