Stack Traces
============

To allow the inclusion of stack traces in exceptions, the standard library provides the ability to
store, inspect, and pretty-print stack traces. This functionality was designed to minimize the cost
of collecting and storing a stack trace, since it is fairly common for a stack trace in an exception
to be captured but never printed. As such, the representation of a stack trace simply contains a set
of pointers to locations in the generated machine code. Formatting into plain text is performed on
demand if required.

The class [stormname:core.StackTrace] stores a stack trace. A stack trace is simply a list of
[stormname:core.StackTrace.Frame] values that each represent a single stack frame. These are
accesible throught the standard operations for arrays (i.e. `empty`, `any`, `count`, and `[]`). Each
stack frame simply stores a pointer to a location in a function. As such, this information is not
very interesting to inspect from Storm.

The following functions are available to create stack traces:

- [stormname:core.collectStackTrace()]

  Creates a stack trace for the current thread as a `StackTrace` object. Typically, the stack trace
  object is stored and then the [stormname:core.StackTrace.format()] function is called to create a
  human-readable representation of the stack trace. The `format` process is relatively expensive, as
  it accesses debug information in order to look up function names and locations. If a stack trace
  is printed without formatting it, it simply prints the raw addresses. As such, capturing a stack
  trace can be done as follows:

  ```bsstmt
  print(collectStackTrace().format());
  ```


For debugging, it is also possible to capture a snapshot of all active threads in the system, or a
subset thereof. It is worth noting that these functions are mainly intended for debugging. They are
therefore aimed at providing helpful output rather than efficiency. As such, while it is possible to
capture a snapshot of the threads in the system to detect if some thread has terminated, it is *not*
advisable as the operation is very expensive. For example, the system traverses the name tree to
look up thread names.

The following functions can be used for this purpose:

- [stormname:core.collectThreadStackTraces()]

  Collects stack traces for all user threads associated with the current OS thread. The returned
  object consists of a list of `StackTrace` objects. Printing it using `toS` produces a
  human-readable representation.

- [stormname:core.collectThreadStackTraces(core.Array(core.Thread))]

  Collects stack traces for all user threads associated with all threads in the `threads` array. The
  returned object is an array of `ThreadStackTraces`. Printing it produces a human-readable
  representation.

- [stormname:core.collectAllStackTraces()]

  Collects stack traces for all user threads associated with all threads in the system.

- [stormname:debug.threadSummary]

  Print a thread summary to standard output. Equivalent to calling
  `print(collectAllStackTraces().toS())`. Note that this function is in the `debug` package.
