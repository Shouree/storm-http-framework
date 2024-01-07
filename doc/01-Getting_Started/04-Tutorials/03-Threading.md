Threading in Storm
==================

This tutorial explores the threading model in Storm. In contrast to most other languages, Storm is
aware of the different threads in a program and is able to use this information to ensure that no
data races are possible.

The code presented in this tutorial is available in the directory `root/tutorials/threads` in the
Storm release. You can run it by typing `tutorials:threads:main` in the Basic Storm interactive
top-loop.

Setup
-----

Before starting to write code, we need somewhere to work. For this tutorial, it is enough to create
a file with the extension `.bs` somewhere on your system. The tutorial uses the name `threads.bs`,
but any name that contains only letters will work. As a starting point, add the following code to
the file:

```bs
void main() {
}
```

After doing this, open a terminal and change to the directory where you created the file. Then run
the file by typing:

```
storm threads.bs
```

Note that based on how you have installed Storm, you might need to [modify the
command-line](md:../Running_Storm/In_the_Terminal) slightly. Also, if you named your file something
different than `types.bs`, you need to modify the name in the command accordingly.

If done correctly, Storm will find your file, notice that it contains a `main` function and execute
it. Since the `main` function does not yet do anything, Storm will immediately exit without printing
anything.


Running Code Concurrently
-------------------------

One of the main benefits of multithreading is the ability to multiple things at the same time. In
Basic Storm, this can be achieved using the `spawn` keyword. To illustrate this, let's start by
defining a function that iteratively computes the sum of the first *n* integers. As a starting point
we artificially slow down the process by calling `sleep` and print the progress:

```bs
Nat computeSum(Nat n, Str progressText) {
    Nat sum = 0;
    for (Nat i = 1; i <= n; i++) {
        sum += i;
        print("${progressText}: at ${i}, sum ${sum}");
        sleep(1 s);
    }
    return sum;
}
```

We can then call the function inside `main` as follows to compute the sum of the first 3 integers:

```bs
void main() {
    Nat sum3 = computeSum(3, "To 3");
    print("Sum of 1 to 3 is: ${sum3}");
}
```

This produces the following output as we would expect:

```
To 3: at 1, sum 1
To 3: at 2, sum 3
To 3: at 3, sum 6
Sum of 1 to 3 is: 6
```

If we would also like to compute the sum of the first 5 integers, we could do that by calling
`computeSum` once more:

```bs
void main() {
    Nat sum3 = computeSum(3, "To 3");
    Nat sum5 = computeSum(5, "To 5");
    print("Sum of 1 to 3 is: ${sum3}");
    print("Sum of 1 to 5 is: ${sum5}");
}
```

As we would expect, this causes the computations to be performed sequentially and we get the
following output:

```
To 3: at 1, sum 1
To 3: at 2, sum 3
To 3: at 3, sum 6
To 5: at 1, sum 1
To 5: at 2, sum 3
To 5: at 3, sum 6
To 5: at 4, sum 10
To 5: at 5, sum 15
Sum of 1 to 3 is: 6
Sum of 1 to 5 is: 15
```

However, since the two calls to `computeSum` are independent of each other, we could perform them
concurrently. In Basic Storm, we can do this by adding the keyword `spawn` before a function
call. This causes the function to be called asynchronously, and as such the expression evaluates to
`Future<T>` instead of just `T`. For example, if we wish to execute the first call to `computeSum`
asynchronously, we can modify the `main` function as follows:

```bs
void main() {
    Future<Nat> sum3 = spawn computeSum(3, "To 3");
    Nat sum5 = computeSum(5, "To 5");
    print("Sum of 1 to 3 is: ${sum3.result}");
    print("Sum of 1 to 5 is: ${sum5}");
}
```

From the code above, we can clearly see that the `spawn` keyword causes the function call to return
`Future<Nat>` instead of just `Nat`. The `Future` object therefore represents the fact that a value
of type `Nat` will be produced at some point in the future, but that it might not be ready yet. The
`Future` type has a member function `result` that can be called to extract the result. If the result
is not yet available, the `result` function waits for the result to be produced. The code above is
therefore safe even if computing the sum of the first 3 integers were to take longer than the
computation of the first 5, or if we wished to use `spawn` for both calls to `computeSum`.

Furthermore, by running the program we find that it completes much quicker than before. We can also
see that execution of the two calls to `computeSum` were executed concurrently by looking at the
order of the printed lines from `computeSum`:

```
To 5: at 1, sum 1
To 3: at 1, sum 1
To 3: at 2, sum 3
To 5: at 2, sum 3
To 3: at 3, sum 6
To 5: at 3, sum 6
To 5: at 4, sum 10
To 5: at 5, sum 15
Sum of 1 to 3 is: 6
Sum of 1 to 5 is: 15
```

It is worth nothing that while the output for this particular example tends to be the same each
time, the order in which the lines appear are actually non-deterministic. The lines may therefore
appear in a different order when you run the program.

In some cases it is useful to start a thread to take care of some task, even though we are not
interested in the result produced by the `spawn`ed function. For example, if we were only interested
in the `print` statements from `computeSum` above, we could simply use `spawn` to start the thread,
but then call `detach` instead of `result` to indicate that we are not interested in the
result. Calling `detach` is technically optional, but doing so makes Storm output any uncaugh
exceptions in the spawned thread immediately, rather than waiting for another thread to call
`result`. Since compilation errors are exceptions in Storm, not calling `detach` in such cases could
mean that compilation errors are not visible until much later. This typically makes it quite
difficult to debug such programs properly.

For example, if the result from the `spawn` expression is stored in a variable (or used in any other
way), we can call `detach` as follows:

```bsstmt
var x = spawn computeSum(3, "To 3");
x.detach();
```

As a special case, if `spawn` appears as a statement where the result is not used or returned, then
the system will automatically call `detach` as a convenience. As such, the following two lines are
equivalent in many situations:

```bsstmt
spawn computeSum(3, "To 3");
(spawn computeSum(3, "To 3")).detach();
```

Multiple Threads
----------------

Storm implements two kinds of threads: *OS threads* and *user threads*. The first kind, *OS
threads*, are threads that are scheduled by the operating system. Different OS threads may therefore
run on different physical CPU cores. User threads on the other hand are implemented inside Storm
(i.e. in *userspace*). They are therefore invisible to the operating system, and the operating
system may not schedule them on different cores.

All user threads belong to a specific OS thread. All user threads that belong to the same OS thread
are scheduled in a cooperative fashion. This means that a user thread will be executed until a point
where it explicitly allows other threads to run. This is nice since it makes it easier to reason
about where threads may be interrupted, but it needs some care with long-running tasks.

In the example above, `spawn` will create different user threads on the same OS thread. As such,
they are scheduled cooperatively on the same OS thread, and are not able to execute in parallel on
multiple cores. The reason the above example works well is that the call to `sleep` (and actually
also `print`) lets other user threads run if necessary.

To illustrate the implications of this, let's implement a simple recursive `fibonacci` function and
measure its execution time. We intentionally use a slow implementation to make it easier to see when
we utilize multiple physical cores or not:

```bs
Word fibonacci(Nat n) {
    if (n < 2)
        return n;
    else
        return fibonacci(n - 1) + fibonacci(n - 2);
}

void main() {
    Moment start;
    Word value = fibonacci(40);
    Moment end;
    print("Computed fibonacci(40)=${value} in ${end - start}");
}
```

In the example we use the `Moment` class to measure execution time. As the name implies, a `Moment`
reprents a moment in time. The `Moment` is initialized to the current time in a high-resolution
system specific clock. We then compute the difference between two `Moment`s to get a `Duration` that
we print.

It is also worth noting that we make `fibonacci` return a `Word`, which is a 64-bit unsigned
integer. This is to avoid overflows for a bit longer than just using a `Nat`.

The execution time will of course vary depending on the speed of your computer. The output on my
machine is the following:

```
Computed fibonacci(40)=102334155 in 2.10 s
```

Let's try to compute both `fibonacci(40)` and `fibonacci(41)`. Of course, we can do it sequentially
as follows:

```bs
void main() {
    Moment start;
    Word value1 = fibonacci(40);
    Moment middle;
    Word value2 = fibonacci(41);
    Moment end;
    print("Computed fibonacci(40)=${value1} in ${middle - start}");
    print("Computed fibonacci(41)=${value2} in ${end - middle}");
    print("Total time: ${end - start}");
}
```

This produces output similar to below. Again, the times are likely to be different on your machine:

```
Computed fibonacci(40)=102334155 in 2.10 s
Computed fibonacci(41)=165580141 in 3.40 s
Total time: 5.49 s
```

Since the two calls to `fibonacci` are independent of each other, we can execute them in parallell
with the `spawn` keyword. Hopefully this allows the computation to finish quicker!

```bs
void main() {
    Moment start;
    Future<Word> value1 = spawn fibonacci(40);
    Future<Word> value2 = spawn fibonacci(41);
    print("Finished fibonacci(40)=${value1.result}");
    print("Finished fibonacci(41)=${value2.result}");
    Moment end;
    print("Total time: ${end - start}");
}
```

Interestingly, the program above does not execute quicker than the sequential program. Why did we
not se any speedup?

The reason is that the threads started by `spawn` start on the same user thread by default. This
means that the two calls to `fibonacci` are sceduled cooperatively on a single OS thread. Since only
OS threads are visible to the operating system, the operating system is unable to schedule the user
threads on different cores. In our case, one call to `fibonacci` will simply run before the other,
just as in the sequential example. As such, different user threads on the same OS thread are good
for concurrent execution of different tasks that are not CPU bound. To achieve parallel execution,
we need to create the user threads on different OS threads. There is a cost associated with this,
which is why it is not done by default.

In Basic Storm, we can declare OS threads using the `thread` keyword as follows:

```bs
thread FibA;
thread FibB;
```

We can then instruct the system that certain functions need to be executed by a particular thread
using the `on` keyword:

```bs
Word fibonacciA(Nat n) on FibA {
    return fibonacci(n);
}

Word fibonacciB(Nat n) on FibB {
    return fibonacci(n);
}
```

This means that the system will ensure that `fibonacciA` is always executed on the thread named
`FibA`, and that `fibonacciB` is always executed on the thread named `FibB`. We can of course still
call the functions normally as follows:

```bs
void main() {
    Word value1 = fibonacciA(40);
    Word value2 = fibonacciB(41);
    // ...
}
```

This is, however, not very useful for us. While the system ensures that the different threads are
used, the program above would first start the computation of `fibonacciA` on the `FibA` thread, then
wait for the computation to complete before starting to compute `fibonacciB`. As such, the above is
equivalent to:

```bs
void main() {
    Word value1 = (spawn fibonacciA(40)).result();
    Word value2 = (spawn fibonacciB(41)).result();
    // ...
}
```

This is not what we want, since our goal is to execute the two functions in parallel. With this
setup we can, however, go back to our previous idea of using `spawn`:

```bs
void main() {
    Moment start;
    Future<Word> value1 = spawn fibonacciA(40);
    Future<Word> value2 = spawn fibonacciB(41);
    print("Finished fibonacci(40)=${value1.result}");
    print("Finished fibonacci(41)=${value2.result}");
    Moment end;
    print("Total time: ${end - start}");
}
```

If we run the program now, we can see that it finishes in around 3.4 seconds instead of almost 6
seconds. This means that we have managed to execute the compuations in parallel at last!


Sharing Data Between OS Threads
-------------------------------

Now that we know how to execute code in parallel, let's explore how Storm makes parallel programming
safe. Remember that Storm guarantees that data races are not possible. However, the following
program looks like it would violate that principle:

```bs
thread MyThread;

class Data {
    Nat value;
}

void modifyData(Data toModify) on MyThread {
    toModify.value = 10;
}

void main() {
    Data data;
    Future<void> modify = spawn modifyData(data);
    data.value = 20;
    modify.result(); // To wait for 'modifyData' to finish.
    print("After modifications: ${toModify.value}");
}
```

Remember that class types have reference semantics. We would therefore expect that `main` and
`modifyData` would have access to the same instance of `Data`. Since they execute in parallel, this
would constitute a data race.

However, if we run the program we will see that it always prints `20`. If we remove the assignment
`data.value = 20` in `main`, we will further see that the program always prints `0`, even though we
ensure that `modifyData` has finished execution before printing the modified value. This seems
strange!

The reason for the behavior above is that Storm enforces that value- and class-types are *not*
shared between different OS threads, since that would risk creating data races. This means that
Storm needs to copy data that may be passed between two thread. This decision is generally made
statically. In this case, the function `main` is not associated with a OS thread, so it may run on
any thread in the system. Since the call to `modifyData` has to run on the thread `MyThread`, Storm
inserts code to send a message to the appropriate thread. As a part of that process, it also makes a
deep copy of any value- and class-types passed to and from the function. We can observe this
behavior by defining a copy-constructor in our class:

```bs
class Data {
    Nat value;

    // We need to define the default constructor explicitly, since it will
    // no longer be generated by default when we define another constructor.
    init() {}

    // The copy constructor.
    init(Data other) {
        init { value = other.value; }
        print("Copied data!");
    }
}
```

If we run the program again, we will see that the string `Copied data!` is printed once, when we
call `modifyData`. This is why the changes to `Data` is not visible in the `main` function as we
initially expected.

This behavior can be summarized as: class-types have by-value semantics when they are passed across
a thread boundary. This is the reason why the `==` operator and hash tables compare the contents of
the class rather than using the identity of the object. The identity of the object might change when
objects cross a thread boundary.

To illustrate this, let's modify the `modifyData` function to actually return the `Data` instance
as well:

```bs
Data modifyData(Data toModify) on MyThread {
    toModify.value = 10;
    return toModify;
}
```

We can then inspect the behavior more closely in our `main` function:

```bs
void main() {
    Data data;
    Future<Data> modify = spawn modifyData(data);
    data.value = 20;
    Data modified = modify.result();
    print("After modifications: ${data.value}");
    print("Returned object: ${modified.value}");
    print("Are they the same? ${data is modified}");
}
```

At this point, we can observe that the copy constructor is called twice: once when the `Data`
instance in the `main` function is passed to `modifyData`, and then again when the instance is
returned back to `main`. The instance returned from `modifyData` has indeed been modified to contain
10 as we would expect. Finally, we can also see that `data` and `modified` do not refer to the same
object, which would be the case if we had not associated `modifyData` with a thread. Remember that
the `is` operator compares object identities.


Sharing Data Safely with Actors
-------------------------------

To make it possible to safely share data between OS threads, Storm provides a third kind of type:
*actors*. An *actor* is similar to a class in that it has by-reference semantics. The difference is
that it retains the by-reference semantics even across thread boundaries. This might initially seem
like it would violate the guarantee of no data races. However, an actor needs to be associated with
a thread. Just like the functions that are associated with a thread, this means that the object may
only be accessed by code running on the associated thread.

This is perhaps best illustrated with an example. Let's make a version of the `Data` class that is
an actor and compare its behavior to the example above. An actor is declared just like a class. The
only difference is that we add the `on` keyword followed by the associated thread:

```bs
class ActorData on MyThread {
    Nat value;
}
```

We then modify the code in the `main` function to use the new type instead:

```bs
void main() {
    ActorData data;
    Future<ActorData> modify = spawn modifyData(data);
    data.value = 20;
    ActorData modified = modify.result();
    print("After modifications: ${data.value}");
    print("Returned object: ${modified.value}");
    print("Are they the same? ${data is modified}");
}
```

If we try to run this code it produces the following error on the line `data.value = 20`:

```
@/home/storm/threads.bs(2105-2110): Syntax error:
Unable to assign to member variables in objects running on a different thread than
the caller. Create a function in the actor that performs the desired operation instead.
```

As the error indicates, Storm has noticed that `data` is an actor associated wit the thread
`MyThread`, while `main` is not necessarily executed on that thread. Therefore, to avoid data races,
Storm does not allow modifying `value` in this manner. As the error message indicates, we need to
create a function that performs the assignment for us instead. Since function calls are dispatched
to the proper thread as a message, this approach will avoid data races.

For now, we can simply remove the assignment to `data.value` to be able to run the program. This is
actually enough, even though we read from `value` in the `print` statements. This does indeed look
like a data race, but Basic Storm allows it anyway. What happens in cases like these is that Basic
Storm generates a function that retrieves the variable, and arranges for that function to be
executed on `MyThread`. This means that it is possible to read member variables from actors as
usual, even though the cost of doing so is much higher, and often involves a copy.

Running the program produces the following output:

```
After modifications: 10
Returned object: 10
Are they the same? true
```

This is the behavior we initially expected from class types. As such, we can conclude that actor
types retain the by-reference semantics even across thread boundaries.

Let's fix the error with the assignment in the `main` function above. The error message told us that
we need to make a function in the actor that modifies the variable for us. In this case, we can make
a function `set` as follows inside `ActorData`:

```bsclass
void set(Nat newValue) {
    value = newValue;
}
```

Then we can replace the removed line `data.value = 20;` with `data.set(20);` in the `main` function.
If we run the program now, it runs without errors and produces the following output (most of the
time):

```
After modifications: 20
Returned object: 20
Are they the same? true
```

It is worth noting that the program is currently non-deterministic. The output depends on whether
the `modifyData` or the `set` function is executed first, and the order of these calls is not
specified since they may execute in parallel. Storm does, however, guarantee that `modifyData` and
`set` are not interleaved with each other, since they are scheduled cooperatively on the same OS
thread.

### Thread-safety

To illustrate the implications of the message-passing model in Storm, let's assume that we wish to
increase `value` with a specific amount from two different threads. We can implement this as
follows:

```bs
thread MyThread;

class ActorData on MyThread {
    Nat value;

    void set(Nat newValue) {
        value = newValue;
    }
}

thread MyOtherThread;

void addData(ActorData toUpdate, Nat toAdd) on MyOtherThread {
    toUpdate.set(toUpdate.value + toAdd);
}

void main() {
    ActorData data;
    Future<void> modify = spawn addData(data, 20);
    for (Nat i = 0; i < 10; i++) {
        data.set(data.value + 10);
    }
    modify.result(); // Wait for completion.
    print("Final result: ${data.value}");
}
```

Since we add 10 to `value` 10 times and 20 once, we would expect this program to always print the
result `120`. This is, however, not the case. If I run the program on my machine, I get the
following output most of the time:

```
Final result: 100
```

What happened here? The issue lies in how we have implemented the increment operation, both in
`main` and in `addData`. As it is written now, we first read from `value`, then add some value to
it, and finally write it back using the `set` function. While Storm provides the guarantee that
reading and writing (using `set`) are not interrupted by other threads, it provides no guarantees
that nothing happens between these operations. As such, what happened above was likely the
following:

- `main` read `value` and got the value 0.
- `addData` read `value` and got the value 0.
- `addData` added 20 to 0 and calls `set(20)`.
- `main` added 10 to 0 and calls `set(10)`, which overwrites the result from `addData`.

To fix this problem, we need to make sure that the read and write to `value` are not interrupted by
other operations. Luckily, it is enough to make sure that the increment operation occurs on the same
OS thread as the one associated with the actor. The easiest way of doing this is to add a member
function that performs the operation that needs to be uninterrupted:

```bsclass
void add(Nat toAdd) {
    value = value + toAdd;
}
```

If we then make sure to call the `add` function wherever we increment the value, the program will
work as expected, and always print `120`:

```bs
void addData(ActorData toUpdate, Nat toAdd) on MyOtherThread {
    toUpdate.add(toAdd);
}

void main() {
    ActorData data;
    Future<void> modify = spawn addData(data, 20);
    for (Nat i = 0; i < 10; i++) {
        data.add(10);
    }
    modify.result(); // Wait for completion.
    print("Final result: ${data.value}");
}
```

As such, we can see that the threading model in Storm generally lets us treat member functions in
actors as "atomic", i.e. that they are not interrupted partway through. This has the nice property
that the goal of providing a nice interface to actor types aligns well with making the interface
thread safe.

There is one exception to this rule. If we call a function on another thread from the member
function of an actor, other function calls may execute at that point. For example, assume that
`threadFn` is such a function. Then, calling `threadFn` inside of `add` like below would make it
possible for the threading issues we saw before to arise:

```bsclass
void add(Nat toAdd) {
    Nat original = value;
    threadFn();
    value = original + toAdd;
}
```

Code like above does, however, make the reader question whether `threadFn` does something to `value`
already, so these types of situations are not typically a big issue.

### Inheritance

Finally, a quick word on inheritance related to actors. Apart from being associated to a thread,
actors behave like classes. As such, it is possible to inherit from actors as well. For example, we
could create an actor that adds a member variable to our `ActorData` as follows:

```bs
class Derived extends ActorData {
    Nat count;
}
```

Since `Derived` inherits from `ActorData` that is an actor, `Derived` will also become an actor that
is associated to the same thread as `ActorData`. This also applies to all types that inherit from
`Derived`. This makes it quire convenient to use and extend functionality from a library without
worrying too much about threading, as we shall see in the following tutorials.
