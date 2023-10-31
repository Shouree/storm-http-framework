Unit Tests
==========

The package `test` contains a library for unit tests, and a language extension for Basic Storm that
allows easy creation of tests and test suites. According to the library, tests are individual assert
statements, that are in turn organized into suites. The suites are simply functions that accept zero
parameters and return a [stormname:test.TestResult]. Tests can be further organized into packages.

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
the keyword `suite` for defining a test suite. Inside a test suite, the `test` keyword becomes
available to test some behavior, and the `abort` statement to abort testing.

Defining a test suite looks as follows:

```bs
use test;

// Create a test suite. Generates a function.
suite MySuite {
    // Check for equality.
    test f(1) == 3;
    // Check for inequality.
    test f(2) < 8;
    // Test boolean expressions.
    test true;
    // Test that a particular exception is thrown.
    test f(3) throws InternalError;
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
