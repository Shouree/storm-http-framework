Threading
=========

Due to the user level threading in Storm, it is necessary to pay extra attention to calling
functions that needlessly block the current OS thread from C++. Generally, such functions are not
visible to Storm, so they don't generally pose a problem to code written in Storm. However, C++
since has access to all system interfaces, it is possible to do the wrong thing.

In generall, it is safe to call any functions and use any classes that are also visible to Storm,
since they are aware of Storm's threading model. Furthermore, the OS library (in the `OS/`
directory) provides low-level synchronization primitives that are aware of the user level threads in
Storm. On the other hand, the synchronization primitives available in the `Util/` directory, are not
aware of the threading library and will block the entire OS thread. Similarly, calling non-async
versions of `read` and `write` may have the same effect.
