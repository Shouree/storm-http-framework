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



- Declaring named threads, executing code on them
- Implications on semantics for class types

Actors: Sharing Data Safely
---------------------------

- Actors
- Talk about inheritance



As mentioned in the [previous tutorial](md:Values_and_Classes), Storm has three kinds of types:
value types, class types, and actor types.


- Example with different threads, and how it affects mutability.
