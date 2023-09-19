Threading Model
===============

Storm supports two kinds of threads: *OS threads* and *User threads*. OS threads are scheduled by
the operating system, and different OS threads are therefore able to execute concurrently, possibly
in parallel on different physical CPU cores. The Storm runtime then schedules one or more user
threads (`UThread`s) on top of each OS thread. These threads are scheduled cooperatively, which
means that user threads that are scheduled on the same OS thread never execute in parallel. This
means that it is often safe to share data between multiple user threads as long as they belong to
the same OS thread. This is, of course, not true for different OS threads. In such cases, additional
synchronization is required.


The type system in Storm is aware of OS threads and aims to ensure that shared data between
different OS threads is handled in a safe way by default. This is achieved by conceptually
associating all data in a program to a particular OS thread, and then only allow OS threads to
access the data associated to them. Threads are then allowed by sending messages to each other. In
Storm, messages are implemented by function calls that are executed on the appropriate OS thread. As
such, sending a message involves spawning a new user thread on the correct OS thread. The user
thread then executes the called function, and may return a result to the caller. To avoid shared
data, all parameters are copied before the function call. Similarly, any value returned from the
function is copied before being returned to the caller.

*TODO:* Integrate into the section on the type system?
