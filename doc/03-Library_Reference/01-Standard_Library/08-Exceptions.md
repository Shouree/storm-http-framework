Exceptions
==========

The standard library provides the type `core.Exception`, which is the root class for all exceptions
in Storm. Each exception contains a *message*, and optionally also a *stack trace*. It has the
following members:

- `message()`

  Retrieve the message in the exception. The function simply calls `message(StrBuf)` to ask derived
  classes to create the message.

- `message(StrBuf)`

  Abstract in `core.Exception`, called to let derived classes create an appropriate error message
  into the provided `StrBuf`.

- `saveTrace()`

  Save a stack trace into this exception object. This is not done by default, since collecting a
  stack trace is a fairly expensive operation. Some categories of exceptions do, however, collect a
  stack trace by default.

  Returns the exception itself, so it is possible to easily modify code from:

  ```bsstmt
  throw MyException();
  ```

  into:

  ```bsstmt
  throw MyException().saveTrace();
  ```

  to temporarily catch and print a stack trace.

- `stackTrace`

  Stores the collected stack trace.

- `toS()`

  Outputs a string representation of the exception. By default, this representation contains the
  message (retrieved by calling `message(StrBuf)`), and the collected stack trace.


Exception Categories
--------------------

The following exceptions are provided in the standard library:

- `RuntimeError` extends `Exception`

  Depicts a runtime error from some part of the system. The following subclasses are provided:

  - `GcError` extends `RuntimeError`

    Thrown when the garbage collector detects an issue. Typically when available memory is low.

  - `NumericError` extends `RuntimeError`

    Thrown when a numeric operation failed.

    - `DivisionByZero` extends `NumericError`

      Thrown whenever division by zero is attempted.

  - `MemoryAccessError` extends `RuntimeError`

    Thrown when an invalid memory address was accessed.

  - `InternalError` extends `RuntimeError`

    Generic form of internal error.

  - `AbstractFnCalled` extends `RuntimeError`

    Thrown when an abstract function was called.

- `NotSupported` extends `Exception`

  Thrown when an operation is not supported.

- `UsageError` extends `Exception`

  Thrown when an interface is used incorrectly.

- `StrError` extends `Exception`

  Errors from `core.Str`.

- `MapError` extends `Exception`

  Errors from `core.Map` and `core.RefMap`.

- `SetError` extends `Exception`

  Errors from `core.Set` and `core.RefSet`.

- `ArrayError` extends `Exception`

  Errors from `core.Array`.
