Exceptions
==========

Exceptions in Basic Storm are thrown using the `throw` keyword. It accepts an arbitrary expression
that is evaluated to the exception object to be thrown. It typically looks as follows:

```bsstmt
throw RuntimeError("Something went wrong!");
```

As noted in the [storm reference](md:/Language_Reference/Storm/Exceptions), all exceptions must
inherit (either directly or indirectly) from the `core:Exception` class. As such, new exceptions can
be defined in Basic Storm, just like a regular class:

```bs
class MyException extends Exception {
    private Str msg;

    init(Str message) {
        init { msg = message; }
        // Save the stack trace of the current thread in the exception.
        // This is optional (as it is an expensive operation), and will
        // cause '.toS' to output the stack trace.
        saveTrace();
    }

    protected void message(StrBuf to) : override {
        to << "My error: " << msg;
    }
}
```

As we can see, the implementation calls `saveTrace` to store a stack trace in the exception. This is
not done automatically, as collecting stack traces is a fairly expensive operation. If the stack
trace is not interesting, this step can be omitted.

Furthermore, to generate a descriptive message, the exception class overrides `message(StrBuf)`
rather than `toS(StrBuf)`. This is to make it possible to extract only the message (without the
stack trace) by calling `message()`. The `toS` implementation will include the stack trace if it is
present.


Exceptions are caught using a `try` block:

```bsstmt
try {
    throw MyException("Test");
} catch (MyException e) {
    print("My exception: ${e}");
} catch (Exception e) {
    print("Generic exception: ${e}");
}
```

The code inside the `try` block is executed as normal. However, if an exception is thrown inside the
block, execution is redirected to one of the `catch` blocks. The system searches the call stack from
the `throw` block and upwards until a suitable `catch` block is found. If multiple `catch` blocks
are present, then they are searched from top to bottom. The example above would thus print `"My
exception: ..."` since the `MyException` handler is before the `Exception` handler. As illustrated
below, `catch` blocks closer to the throw site have priority:

```bs
void main() {
    try {
        a();
        print("OK!");
    } catch (MyException e) {
        print("My exception: ${e}");
    } catch (Exception e) {
        print("Generic exception: ${e}");
    }
}

void a() {
    try {
        b();
        print("A OK!");
    } catch (Exception e) {
        print("Generic exception in a: ${e}");
    }
}

void b() {
    throw MyException("Test");
}
```

This example will print two lines. The first contains `Generic exception in a: ...` and the second
contains `OK!`. This means that the general `catch` block inside `a` takes precedence over the
specific one in `main` since it is closer to the site where the exception was thrown.


It is possible to omit the variable name (`e` in this case) if the code does not need to
inspect the caught exception.

It is of course possible to re-throw the caught exception in the handler by using the `throw`
keyword. Since all exceptions are classes, they are handled by reference, and there is therefore no
need for a special form of the `throw` keyword for this situation.
