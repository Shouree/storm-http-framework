Threads
========

Storm knows about two levels of threads: OS threads and user level threads (UThreads). OS threads
are of course scheduled and managed by the operating system, while UThreads are scheduled
cooperatively by the Storm runtime.

Storm attempts to eliminate common threading problems by enforcing no shared memory between
threads. To do this, the compiler needs knowledge about existing threads, and which thread a each
function will be executed on. Therefore, each function in the compiler has an associated thread,
which can be one of these three:

* any thread
* named thread
* unknown compile-time

Any thread means that it does not matter which thread the function is executed on. This means that
the function is executed on the caller's thread, to avoid overhead. When a function is declared to
be executed on a named thread, it means that the compiler will ensure that the function will be
executed on that specific thread (language implementations can currently ignore this limit, may or
may not be altered). A named thread is an OS thread previously declared with a name to the
compiler. The compiler creates a thread for itself named `Compiler`, which the entire compiler uses
to execute. The third possibility is when the actual thread varies runtime, and can not be
inferred. This is used with actor objects where the thread is supplied runtime. It works much like
named threads, but the compiler has to be a little more defensive when calling functions.

To ensure that all functions are executed on their specific OS thread, the compiler may implement
certain function calls by sending a message to another thread. In Storm, this means that a new
UThread is allocated for the target OS thread, and that the calling thread waits for the call to
complete. To ensure that no memory is shared between threads, objects are always deeply copied when
they are sent through a message (and on the way back, through a `Future`). There is one exception
from this rule: actors. Since actors work by sending and receiving messages, and not by reading and
writing directly to them, actors are not strictly shared memory. Therefore, actors are not copied
when sent through a message. This allows communication between threads while avoiding many of the
headaches that may occur when using (unintentional) shared memory.

As mentioned earlier, each message is implemented by spawning a new UThread on the specific OS
thread, which means that each OS thread may have more than one running thread. Why does Storm allow
multiple UThreads potentially sharing data? Won't that break the no shared memory policy? In a way
it does, but it all boils down to the different scheduling of UThreads with respect to OS
threads. UThreads are scheduled cooperatively, which means that one UThread must explicitly `yield`
in order for another UThread to be able to execute. This means that we know exactly where potential
thread switches occur, and reasoning about your program becomes much easier. Most languages only
allow thread switches when sending a message and waiting for the result. Thread switches may also
happen when waiting for results in a `Future`, or when playing with locks. This means that the only
places we have to worry about someone else is playing with our memory is during function calls,
everything else can be considered an atomic operation with respects to other threads running on the
same OS thread (those are the only one we have to worry about since those are the only ones we share
memory with). If you think about it, this is not much worse than coding single-threaded. As long as
you are not calling a function, you know exactly what is happening, but when you do, you have to
make sure that the other function does not destroy your data in some way. The only difference here
is that something completely unrelated can happen when you call a function.
