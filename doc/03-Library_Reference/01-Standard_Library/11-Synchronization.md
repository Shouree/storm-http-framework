Synchronization
===============

Storm is aware of threads by default, and aims to avoid data races. To achieve this, Storm utilizes
some mechanisms in the standard library. In particular, the classes `core.Thread` and
`core.Future<T>` are central to making inter-thread communication happen.

Even though Storm avoids data races by default, it is sometimes necessary to explicitly synchronize
code. This is particularly true when integrating external libraries in C++. As such, Storm provides
a number of synchronization primitives that can be used for these purposes.


Thread
------

The [stormname:core.Thread] class represents an OS thread in Storm. As such, different threads may
execute in parallel at the discretion of the operating system. Threads are typically created lazily.
This means that the actual OS thread is not created until work is submitted to it. The thread is
then kept alive and accepts work until the associated `Thread` object has been destroyed. At that
point it executes until its work-queue is empty and then terminates. In Storm's threading model, it
is common for threads to be created soon after startup, and then kept alive until the application
ends (since they are bound to `NamedThread` entities).

The thread class itself does not contain facilities to submit work to a thread. Rather, one needs to
use the language facilities to submit work to threads (e.g. the `spawn` keyword in Basic Storm, or
creating an actor associated with the thread).

The `Thread` class has the following members:

```stormdoc
@core.Thread
- .__init()
- .==(*)
- .hash()
```


Future
------

As mentioned above, the `core.Future<T>` class is central for inter-thread communication. A future
represents an object of type `T` that will be present at some time in the future. The future is
expected to be populated by some particular thread (typically the thread that received a message),
and other thread(s) may wait for the value to become available (typically the thread that sent the
message). Since a future may be used to wait for values, it is possible to set `T` to `void` to
indicate that one is only interested in waiting for an action to complete, but not the actual
object.

In addition to the result `T`, a future may also store the result in form of an exception. This
causes the future to throw the exception whenever a thread asks for the result. As such, this
behavior is used to propagate exceptions across thread boundaries.

Since future objects are thread-safe, it is safe to share them between different threads. Because of
this, making a copy of a future object will create a copy that refers to the same internal
representation. This means that all copies are linked, and can be used interchangeably. This also
means that it is possible to pass a future object to other threads, even though parameters are
copied.

The `Future<T>` class has the following members:

- `void post(T value)`

  Save `value` in the future. This makes any threads waiting for the value wake up. It is only
  possible to call `post` or `error` once for each future.

- `void error(Exception exception)`

  Save `exception` as an error in the future. This makes any threads waiting for the value to wake
  up. It is only possible to call `post` or `error` once for each future.

- `T result()`

  Wait until the result is available, and return it. If the result posted was an error, it is thrown
  by the `result` function. It is possible to call `result` any number of times.

- `void errorResult()`

  Like `result`, but only waits for a result to become available and throws an error if appropriate.
  The result stored in the future (if any) is not returned.

- `void detach()`

  Informs the future that no more threads will call `result` or `errorResult`. This means that any
  internal data structures can be freed prematurely.

  This is typically used when spawning a thread when one does not wish to wait for the result. This
  call informs the system that any results from the spawned thread can be discarded. It also causes
  the future to output any errors produced by the detached future, as they would otherwise appear to
  be silently ignored by the system.


Locks
-----

The simplest synchronization primitive available in Storm is a recursive lock (sometimes called a
recursive mutex). This is implemented by the class [stormname:core.sync.Lock]. The lock itself is
thread-safe, and therefore all copies of a `Lock` object continue to refer to the same lock. This
makes it possible to share the lock between different threads in Storm. Furthermore, the lock is
aware of Storm's threading model, which means that it only blocks the currently active user-mode
thread.

The lock does not provide the any public members except a default constructor, and logic for copying
the lock. Acquiring and releasing the lock is managed by the nested type
[stormname:core.sync.Lock.Guard]. This type takes a lock as a parameter to its constructor. The lock
is then acquired in the constructor and released in the destructor. As such, assuming that the
`Guard` object is stack-allocated, then releasing the lock will be performed automatically. In Basic
Storm, using a lock may look like this:

```bs
use core:sync;

void function(Lock lock) {
    // Do something that does not require the lock...
    {
        Lock:Guard guard(lock);
        // The lock is now held, manipulate shared data.
    }
    // The lock is now released as the 'guard' is not in scope anymore.
    // Do something that does not require the lock...
}
```


Semaphores
----------

Semaphores provide more flexibility compared to locks. It is, however, easier to cause deadlocks
when using semaphores. In Storm, the class [stormname:core.sync.Sema] implements a semaphore. As
with locks, the semaphore is thread-safe, and therefore all copies of a `Sema` object continue to
refer to the same semaphore. It is thereby possible to share semaphores between threads.
Furthermore, the lock is aware of Storm's threading model, which means that it only blocks the
currently active user-mode thread.

A semaphore can be thought of as an integer variable that needs to be positive. The semaphore
provides two operations that modify the integer variable, `up` and `down`. The `up` operation
increment the integer. This operation never has to wait, as there is no risk of the variable
becoming negative. The `down` operation attempts to decrement the integer. To avoid it becoming
negative, it waits if the integer was zero already. Future calls to `up` will cause it to wake and
succeed in decreasing the integer.

The semaphore has the following members:

```stormdoc
@core.sync.Sema
- .__init()
- .__init(Nat)
- .up()
- .down()
```


Events
------

Events provide a convenient way to wait for some event to occur. In contrast to futures, events can
be reset and reused without creating and distributing new event variables. As such, an event can be
thought of as a boolean variable that is either `true` or `false`. Whenever the variable is `false`,
all threads that call `wait` will wait until the variable becomes `true`. When the variable is
`true`, then threads never wait, and any waiting threads will continue executing. Events can
therefore be used to implement condition variables.

In Storm, events are implemented by the class [stormname:core.sync.Event]. As with the other
synchronization primitives, copies of an event refer to the same underlying variable. It is
therefore possible to share event variables between threads. Furthermore, the lock is aware of
Storm's threading model, which means that it only blocks the currently active user-mode thread.

An event has the following members:

```stormdoc
@core.sync.Event
- .__init()
- .set()
- .clear()
- .wait()
- .isSet()
```
