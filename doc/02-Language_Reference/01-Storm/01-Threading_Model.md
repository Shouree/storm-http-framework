Threading Model
===============

There are two kinds of threads in Storm:

- **OS threads**: Threads scheduled by the operating system. As such, they may be executed in
  parallel by different physical CPU cores. It is therefore generally not safe to share data between
  different OS threads without synchronization.

- **User threads**(`UThread`s): Threads implemented in userspace by the Storm runtime. Each UThread
  belong to an OS thread, and may only be executed by that particular OS thread. User threads are
  cooperatively scheduled, which means that it is often safe to share data between UThreads that are
  belong to the same OS thread. However, some care needs to be taken to ensure that operations that
  need to appear as a unit are not accidentally interrupted by a yield. This is easily done by
  not calling functions that may yield during such times (e.g. avoid reading/writing from files).


The type system in Storm is aware of OS threads, and uses the information to help the programmer to
avoid data races. Conceptually, all memory in the system is associated with a OS thread, and only
the associated OS thread is allowed to access the memory. In this way, the system ensures that no
data is shared between different OS threads.

OS threads are able to communicate by sending each other messages. In Storm, these messages take the
form of function calls. Messages are thus sent by asking another OS thread to call a function with
some parameters. To ensure that no data is shared, a copy of all parameters is made before the
function is called. The sender of the message might chose to wait for the result of the function
call. Similarly to the parameters, the result is also copied before being passed to the caller. Each
such message is executed by creating a new UThread that belongs to the target OS thread. The UThread
is responsible for executing the function and exits when the function returns. Since thread
execution is handled by the scheduler in Storm, it is not necessary to for OS threads explicitly
receive messages.

As previously mentioned, multiple OS threads are not allowed to share memory in this model. The
model does, however, allow multiple OS threads to share references to data, regardless of which OS
thread the data is associated to. While only the associated thread is allowed to access the data,
this allows other threads to ask the proper OS thread to access the data on their behalf in a
convenient manner. As we shall see, this allows implementing *actors* to make communication
convenient.

Finally, it is worth mentioning that the above memory model is what Storm enforces by default. It is
intended to make it easy for programmers to create safe multithreaded programs. It is, however,
possible for language implementations (and indeed programmers) to bypass the model if necessary. In
fact, the standard library bends the rules a bit: since the string type is immutable, it does not
need to be copied between different threads, and is instead shared.
