Unit tests
===========

The package `test` contains a set of functions and a language extension for Basic Storm that allows
easy creation and execution of unit tests. With the language extension, tests are written as follows:

```
suite MySuite {
    test 1 + 2 == 3;
    test 2 + 5 < 8;
    test true;
    test foo() throws InternalError;
}
```
The test language also introduces the `abort` statement, which can be used to abort the remainder of
the suite if something went very wrong. This also inhibits executing any further thests.

The tests can then be executed by running the following code either in the interactive shell, or
somewhere else (after using `lang:bs:macro` for `named`):

```
test:runTests(named{package});
```

Where `package` is the name of the package where the suite(s) to execute are located. Running tests
like this either produces output telling you that all is well, or diagnostic messages of the
following format:

```
Failed: foo(1) == 3 ==> 2 == 3
```

Which means that the test was `foo(1) == 3`, but the testing framework observed the check `2 == 3`,
which is obviously wrong. In this case, it means that `foo(1)` returned `2` rather than `3`.

The tests can also be executed from the command-line using the `-t` or `-T` parameter as follows:

```
./Storm -t test.example
```

Where `-t` executes all tests in the indicated package only, and `-T` executes tests found in
sub-packages as well.
