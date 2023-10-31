Debugging Storm
===============

This page details some debugging strategies that are useful when debuggin Storm and C++ code,
especially when mixed. The page also details some additional quirks due to how the garbage collector
behaves, and how to deal with them.


Signals and Exceptions
----------------------

The garbage collector occasionally write protects data that is usually writable, so that it may be
notified when data is accessed, and thereby avoid scanning parts of the heap. This is sometimes
confusing during debugging, as a seemingly innocent write to memory may suddenly jump into the
garbage collector. On Windows, this is typically not a problem, due to the existence of the
system-wide exception handling mechanisms. On Linux, however, an access violation sends the SIGSEGV
signal to the process, which typically traps the debugger even if the signal is later handled.

As such, on Linux it is useful to tell the debugger to ignore the `SIGSEGV` signal. This can be done
by issuing the following command, either interactively or in a `.gdbinit` file.

```
handle SIGSEGV nostop noprint
```

If it is interesting to debug genuine segmentation faults, one can instead put a breakpoint on the
function `handleSegv` in `Compiler/SystemException.cpp`. This function is only called in the case of
a "true" segmentation fault. Similarly, on Windows, the function `throwAccessError` in the same file
serves the same purpose.


On Linux, a further two signals are used to coordinate threads, namely `SIG34` and `SIG35`. These
signals should therefore be ignored in the same manner when using GDB:

```
handle SIG34 nostop noprint
handle SIG35 nostop noprint
```

Due to the use of signals, it is of extra importance to pay attention to the return values from
system calls that may fail with `errno` set to `EINTR`. These situations tend to happen more often
in Storm than in other programs due to the garbage collector, even though the signals are handled
with the *resume system calls* flag set.


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
