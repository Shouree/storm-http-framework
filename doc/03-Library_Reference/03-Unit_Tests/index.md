Unit Tests
==========

The package `test` contains a library for unit tests, and a language extension for Basic Storm that
allows easy creation of tests. Each test is essentially a function without parameters that may
contain `check` statements. Each test is callable as a function that returns a
[stormname:test.TestResult] that contains the result of the test-run. Tests that are located in the
same package can be thought of as a test suite.

As mentioned in the [language reference](md:/Language_Reference/Storm/Command_Line), it is possible
to run test suites using the `-t` or `-T` parameter on the command line.


Library
-------

The library provides a number of types and functions that support the creation and execution of test
suites. In particular, the type [stormname:test.TestResult] represents the result of executing one
or more test suites. It has the following members:

```stormdoc
@test.TestResult
- .total
- .failed
- .crashed
- .aborted
- .ok
- .add(*)
```

As with other types, [stormname:test.TestResult] can be printed. This produces a summary of the test
results.

The library also contains a number of functions to execute test suites:

```stormdoc
- test.runTests(*)
```


Language Extension
------------------

As mentioned above, the package also contains a language extension for Basic Storm. It introduces
the keyword `test` for defining a test. Inside a test, the `check` keyword becomes available to
check some behavior (i.e. similar to `assert` in some libraries), and the `abort` statement to abort
testing.

Defining a test suite looks as follows:

```bs
use test;

// Create a test suite. Generates a function.
test MySuite {
    // Check for equality.
    check f(1) == 3;
    // Check for inequality.
    check f(2) < 8;
    // Test boolean expressions.
    check true;
    // Test that a particular exception is thrown.
    check f(3) throws InternalError;
}

// Function to test, can be located elsewhere.
Nat f(Nat x) {
    if (x >= 3)
        throw InternalError("Error");
    return x + 1; // change to 2 to make tests pass
}
```

As can be seen above, the `test` keyword recognizes the standard comparison operators in Basic Storm
(i.e. `==`, `!=`, `>`, `<`, `>=`, `<=`, `is`, `!is`). Operators added via extensions might not be
visible. Adding operators for the test syntax requires extending the `test.STestOp` production. When
any of them fails, it will print a diagnostic message of the following form:

```
Failed: fn(1) == 3 ==> 2 != 3
```

As can be seen, the left-hand side of the arrow `==>` prints the original expression, and the right
hand side prints the actual values of the expression, along with the inverse of the operator to
illustrate the issue.
