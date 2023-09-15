Debugging
==========

There are a few utilities in the C++ code that can be used to make debugging easier. Since Storm
generates code, it is harder to use some functionality in C++ debuggers as usual. For example, stack
traces from within debuggers are unaware of the function names for generated code. Therefore, it is
not possible to step through Storm code in source mode, or set breakpoints in Storm code easily.


Breakpoints
------------

The function `storm::dbgBreak()` is useful for breaking the program at an arbitrary point. It is
exposed to Storm as `core.debug.dbgBreak()`, which makes it useful in Storm code as well.

On Linux, the function raises the signal `SIGINT`, which is easily captured inside GDB.

On Windows, it calls the function `DebugBreak()`, which breaks into the debugger. Historically, this
would bring up the dialog "This program has stopped working..." which would give you the option to
open a debugger (sometimes called Just In Time or JIT debugging). Recently, it seems like this is
disabled by default. To use this method you therefore either need to run Storm under a debugger
(e.g., launch it from Visual Studio), or modify the registry to enable JIT debugging. In the
repository, the file `scripts/ShowCrashUI.reg` contains the keys that need to be modified.


Stack traces
-------------

To print a stack trace, call the function `storm::stackTrace()` from C++ or
`core.debug.stackTrace()`. The stack trace contains function names from both C++ and from inside
Storm. This function is not available inside shared libaries. In this case, use
`collectStackTrace(<engine>).format()` from the file `Core/StackTrace.h` instead. The function
`collectStackTrace` is also available from Storm. It returns a stack trace object that can be saved
for later inspection (e.g., in an exception). Similarly, many exceptions collect stack traces
automatically when they are created.


Signals on Linux
-----------------

There are a few additional things one need to consider when debugging Storm on Linux. Since the
garbage collector uses signals to synchronize scanning of the thread's stacks (the first two
real-time signals, typically SIG34 and SIG35), GDB will halt the program execution when that
happens. Furthermore, the garbage collector utilizes memory protection features in Linux to detect
when data has been read or written, which can be used to avoid doing unnecessary work. The downside
of this is that SIGSEGV signals are generated and handled during normal program operation. Thus,
SIGSEGV can not be used to find memory errors. To simplify debugging, Storm installs another signal
handler for SIGSEGV that is executed if the garbage collector determined that the SIGSEGV
represented a genuine segmentation fault. This handler throws Storm exceptions by default, and
raises a SIGINT if it is not able to do so.

To make GDB ignore the signals used with the garbage collector, put the following in your `.gdbinit`
file, or type them into the interactive prompt when you start GDB (also available in the repository):

```
handle SIGSEGV nostop noprint
handle SIG34 nostop noprint
handle SIG35 nostop noprint
```


Debugging Memory Issues
-----------------------

Since the garbage collector utilizes memory protection, it is sometimes difficult to distinguish
genuine protection errors from false ones. To help debugging, Storm installs a layer of its own
fault handlers for this type of events in the file `Compiler/SystemException.cpp`. These get called
whenever the garbage collector encounters a fault that represents a genuine protection fault that it
is unable to handle. As such, when debugging memory errors it is typically useful to set a
breakpoint on the function `handleSegv` on Linux (type `b handleSegv` in GDB), or the function
`throwAccessError` on Windows.


Generated code
---------------

If you are developing languages or language extensions, it can sometimes be convenient to see the
final machine code that is generated instead of the intermediate representation. This can be done by
the following C++ code:

```
// See the transformed machine code (l is a Listing):
PLN(engine().arena()->transform(l, null));

// See the binary representation as well:
PLN(new (engine()) Binary(engine().arena(), l, true));
```