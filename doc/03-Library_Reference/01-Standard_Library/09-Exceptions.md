Exceptions
==========

The standard library provides the type `core.Exception`, which is the root class for all exceptions
in Storm. Each exception contains a *message*, and optionally also a *stack trace*. It has the
following members:

```stormdoc
@core.Exception
- .message(*)
- .stackTrace
- .saveTrace()
```

The `saveTrace` function returns the exception itself. This makes it convenient to add a stack trace
to an exception temporarily by modifying code like:

```bsstmt
throw MyException();
```

into:

```bsstmt
throw MyException().saveTrace();
```


Exception Categories
--------------------

The following exceptions are provided in the standard library:

- [stormname:core.RuntimeError] extends `Exception`

  Depicts a runtime error from some part of the system. The following subclasses are provided:

  - [stormname:core.GcError] extends `RuntimeError`

    Thrown when the garbage collector detects an issue. Typically when available memory is low.

  - [stormname:core.NumericError] extends `RuntimeError`

    Thrown when a numeric operation failed.

    - [stormname:core.DivisionByZero] extends `NumericError`

      Thrown whenever division by zero is attempted.

  - [stormname:core.MemoryAccessError] extends `RuntimeError`

    Thrown when an invalid memory address was accessed.

  - [stormname:core.InternalError] extends `RuntimeError`

    Generic form of internal error.

  - [stormname:core.AbstractFnCalled] extends `RuntimeError`

    Thrown when an abstract function was called.

- [stormname:core.NotSupported] extends `Exception`

  Thrown when an operation is not supported.

- [stormname:core.UsageError] extends `Exception`

  Thrown when an interface is used incorrectly.

- [stormname:core.StrError] extends `Exception`

  Errors from `core.Str`.

- [stormname:core.MapError] extends `Exception`

  Errors from `core.Map` and `core.RefMap`.

- [stormname:core.SetError] extends `Exception`

  Errors from `core.Set` and `core.RefSet`.

- [stormname:core.ArrayError] extends `Exception`

  Errors from `core.Array`.
