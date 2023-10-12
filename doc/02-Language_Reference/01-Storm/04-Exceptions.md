Exceptions
==========

All exceptions in Storm inherit from the class `core.Exception`, which is a class-type. The
`core.Exception`-class is abstract, and expects derived classes to override the `message(StrBuf)`
function, which is to provide a custom message for the exception when printed.

Exceptions do not record a stack trace by default since this is a fairly expensive operation. The
`core.Exception` class contains a `saveTrace` function that records a stack trace into the exception
that will be printed by the default `toS` implementation. Many of the built-in exceptions in Storm
represent events that are fatal. They are therefore expected to occur rarely, and therefore they
collect the stack trace by default. Therefore, classes inheriting from these classes do not
need to call `saveTrace` again.

The class `core.Exception` separates its string representation from the message in the exception.
Calling `toS` on an exception will include the message and the stack trace if it was captured. It is
also possible to call `message` to only retrieve the message. As with `toS`, subclasses are expected
to override the `message(StrBuf)` function to generate the message.

Storm defines the following generic exception classes:
- `RuntimeError`: Generic runtime errors originating from the runtime system
  - `GcError`: Exception thrown by the garbage collector. Typically due to allocation failures (i.e., out of memory).
  - `NumericError`: Error in numerical operations.
    - `DivisionByZero`: Thrown on division by zero. Note: can not necessarily be caught in C++.
  - `MemoryAccessError`: Thrown on invalid memory accesses. Note: can not necessarily be caught in C++.
- `NotSupported`: Thrown from interfaces where the requested operation is not supported.
- `UsageError`: Thrown on invalid usage of some part of the system.
- `InternalError`: Internal errors from the compiler or runtime system.
- `StrError`, `MapError`, `SetError`, `ArrayError`: Errors thrown by the standard containers if they
  are used incorrectly (e.g. accessing elements that don't exist).
- `CompilerError`: Base class for exceptions from the compiler.
  - `CodeError`: Base class for exceptions that originate from some location in the source code.
    - `SyntaxError`: Class that describes a syntactical error in the source code.
    - `TypeError`: Class that describes a type error in the source code.
