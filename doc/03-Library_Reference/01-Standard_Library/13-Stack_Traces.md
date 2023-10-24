Stack Traces
============

To allow the inclusion of stack traces in exceptions, the standard library provides the ability to
store, inspect, and pretty-print stack traces. This functionality was designed to minimize the cost
of collecting and storing a stack trace, since it is fairly common for a stack trace in an exception
to be captured but never printed. As such, the representation of a stack trace simply contains a set
of pointers to locations in the generated machine code. Formatting into plain text is performed on
demand if required.

The class [stormname:core.StackTrace] stores a stack trace. A stack trace is simply a list of
[stormname:core.StackTrace.Frame] values that each represent a single stack frame. Each stack frame
simply stores a pointer to a location in a function. As such, this information is not very
interesting to inspect from Storm.

A stack trace is created using the function [stormname:core.collectStackTrace()]. The function
captures and returns a `StackTrace` object. Typically, the stack trace object is stored and then the
[stormname:core.StackTrace.format()] function is called to create a human-readable representation of
the stack trace. The `format` process is relatively expensive, as it accesses debug information in
order to look up function names and locations. If a stack trace is printed without formatting it,
it simply prints the raw addresses.

The [stormname:core.StackTrace] class also contains operations to access the individual frames (i.e.
`empty`, `any`, `count` and `[]`).
