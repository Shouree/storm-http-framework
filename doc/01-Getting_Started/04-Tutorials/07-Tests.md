Tests
=====

This tutorial shows how the [unit test library](md:/Library_Reference/Unit_Tests) can be used to
write and run tests.

The code presented in this tutorial is available in the directory `root/tutorials/unit` in the
Storm release. You can run it by passing the `-t` flag to Storm: `storm -t tutorials.unit`


Setup
-----

First, we need to create somewhere to work. For this tutorial, it is enough to create a file
`unit.bs` somewhere on your system. The file can be empty for the time being.

After creating the file, open a terminal and change to the directory where you created the file.
Then run it by typing:

```
storm unit.bs -t unit
```

If done correctly, Storm should print the message `Passed all 0 tests` and exit. Note that based on
how you have installed Storm, you might need to [modify the
command-line](md:../Running_Storm/In_the_Terminal) slightly.

The first part of the command line (`unit.bs`) tells Storm to load the file `unit.bs` into the
package `unit`, as we have done in previous tutorials. By default, this will cause Storm to look for
a function named `main` in the loaded code. However, the `-t` flag overrides this behavior. The flag
`-t unit` asks Storm to run all unit tests in the `unit` package that was just loaded, instead of
attempting to run the function `main`.


Writing Tests
-------------

To make things more interesting, let's start by writing a small function to test. For simplicity, we
will use the iterative implementation of the fibonacci function below:

```bs
Int fibonacci(Int n) {
    Int current = 1;
    Int previous = 1;

    for (Int i = 1; i < n; i++) {
        Int next = current + previous;
        previous = current;
        current = next;
    }

    return current;
}
```

Some readers may already have noticed that the implementation above does indeed not work as we would
expect. Let's create some tests to illustrate this!

The unit test library is built around creating *tests*. Each test is essentially a function that may
contain `check` statements. A test is defined using the `test` keyword, followed by a name for the
test. To be able to use this syntax in Basic Storm, the `test` library must first be included. As
such, we add the use statement to the top of our file:

```bs
use test;
```

And then we can define a suite towards the bottom of the file:

```bsfragment:SFile:use=test
test Fibonacci {
    check fibonacci(1) == 1;
    check fibonacci(2) == 1;
    check fibonacci(3) == 2;
}
```

Now, we can run the test using `storm unit.bs -t unit` to see if our implementation works as
we expect. Sadly, this is not the case as Storm produces the following output:

```
Running Fibonacci...
Failed: fibonacci(2) == 1 ==> 2 == 1
Failed: fibonacci(3) == 2 ==> 3 == 2
Passed 1 of 3 tests
Failed 2 tests!
```

The output ends with a summary that states that only one of our three check statements succeeded, and
that 2 failed. Above, we see the lines `Failed:` that indicate which tests failed. These lines both
start by reproducing the expression that was tested. Then it outputs a long arrow (`==>`), followed
by a representation of what the left- and right-hand side of the operator evaluated to. In the case
of `fibonacci(2) == 1`, the left-hand side was `2` and the right-hand side was `1`. This expansion
is to make it easier to see why the test failed.

In our case, the reason is that the `fibonacci` implementation is "shifted" by one position. That
is, `fibonacci(0)` evaluates `fibonacci(1)` and so on. We can fix this by modifying the
initialization of `previous` as follows:

```bs
Int fibonacci(Int n) {
    Int current = 1;
    Int previous = 0;

    for (Int i = 1; i < n; i++) {
        Int next = current + previous;
        previous = current;
        current = next;
    }

    return current;
}
```

With this modification, Storm will now print:

```
Passed all 3 tests
```


Testing for Exceptions
----------------------

Let's assume that we wish to extend our implementation of `fibonacci` to also support the case where
`n` is zero, and to properly reject negative inputs.

Note: A better way to clearly signal that negative inputs are invalid would be to change the
function to accept `Nat` instead of `Int`. There are, however, cases where this is not as easy as
for the case of `fibonacci`, so we accept `Int` as input to illustrate how to use exceptions in the
test library.


We can easy add a check statement for the case where `n` is zero as follows:

```bsfragment:SFile:use=test
test Fibonacci {
    check fibonacci(0) == 0;
    check fibonacci(1) == 1;
    check fibonacci(2) == 1;
    check fibonacci(3) == 2;
}
```

The test currently fails, but we can easily fix it by adding a special case for zero as follows:

```bs
Int fibonacci(Int n) {
    if (n == 0) {
        return 0;
    }

    Int current = 1;
    Int previous = 0;

    for (Int i = 1; i < n; i++) {
        Int next = current + previous;
        previous = current;
        current = next;
    }

    return current;
}
```

In the case where `n` is below zero, we expect `fibonacci` to throw the exception `NotSupported`. We
can test for this case using the keyword `throws` in a test statement as follows:

```bsfragment:SFile:use=test
test Fibonacci {
    check fibonacci(0) == 0;
    check fibonacci(1) == 1;
    check fibonacci(2) == 1;
    check fibonacci(3) == 2;

    check fibonacci(-1) throws NotSupported;
}
```

If we run the test currently, we will see that the new test fails with the following output:

```
Running Fibonacci...
Failed: fibonacci(-1) ==> did not throw NotSupported as expected.
Passed 4 of 5 tests
Failed 1 test!
```

As we can see above, the message follows a similar structure to before, but here it prints that
`NotSupported` was not thrown rather than expanding the expression. We can fix the issue by adding
an appropriate check to the start of our `fibonacci` function:

```bs
Int fibonacci(Int n) {
	if (n == 0) {
		return 0;
	} else if (n < 0) {
		throw NotSupported("n must not be negative");
	}

	Int current = 1;
	Int previous = 0;

	for (Int i = 1; i < n; i++) {
		Int next = current + previous;
		previous = current;
		current = next;
	}

	return current;
}
```

And with this change, all tests pass again.


Running Tests Programmatically
------------------------------

So far we have used the `-t` flag on the command line to run the tests. It is of course possible to
interact with the tests programmatically as well.

Each test is represented as a regular function in the name tree. As such, we can simply run
the function to run the tests inside of it:

```bs
void main() {
    TestResult result = Fibonacci();
    print("Failed: ${result.failed}");
    print(result.toS);
}
```

As can be seen above, test suites return an instance of the `TestResult` class that contains
information about the tests that were executed. It contains member variables that count the number
of tests executed, the number of failures, and so on. The code above prints the number of failed
tests, and then prints the string representation of the entire `result` object. If we run the
program using `storm unit.bs` (i.e. without `-t unit`), the `main` function will be executed, and we
will see the following output:

```
Failed: 0
Passed all 5 tests
```

As we see from the output, the string representation of the `TestResult` object is what was printed
by the test library automatically. As such, it is quite easy to implement custom logic for running
individual test suites.

It is also possible to run sets of test suites automatically by using the `runTests` function
provided by the library. The function accepts the name of a package that contains the tests that we
wish to execute, and optionally also parameters specifying the verbosity of the run. Since the
function accepts a `Package` instance, it is convenient to use the `named{}` construct from
`lang:bs:macro` to get the current package:

```bsfragment:SFile:use=test:use=lang.bs.macro
void main() {
    TestResult = runTests(named{});
    print("Failed: ${result.failed}");
}
```

If we run this code, we can see that the `runTests` function produces some additional status
messages and prints the result automatically by default. This can of course be disabled by passing
`false` as the second parameter to the function:

```
Running Fibonacci...
Passed all 5 tests
Failed: 0
```


Final Notes
-----------

It is worth noting that `check` statements can appear anywhere inside a test function. This means
that it is possible to use loops and if-statements to programmatically determine which checks should
be executed. We could, for example, do the following to verify that `fibonacci` throws an exception
for many negative numbers if we wished to (the benefit is, however, debatable):

```bsfragment:SFile:use=test
test Negative {
    for (Int i = -1; i > -10; i--) {
        check fibonacci(i) throws NotSupported;
    }
}
```

In other cases, the code in a test might realize that the implementation is too broken to even
execute tests. In situations like this, it is possible to use the `abort;` keyword to abort the test
altogether.

Finally, it is worth noting that it is not necessary to place the tests in the same file as the code
that is being tested. As with most things in Storm, the test suite can be placed anywhere that is
convenient. As such, it is often beneficial to place tests in a sub-package named `tests`. This
makes it so that Storm does not have to compile the tests whenever the tested code is used. Test
cases can even be structured into a hierarcy of packages, where each package represents a suite of
tests. In such cases, the `-t` flag has to be replaced by `-T` in order to instruct the system to
traverse packages recursively.
