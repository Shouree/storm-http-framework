Global Variables
================

A global variable is defined as follows:

```
<type> <name> on <thread name> [= <initializer>];
```

Where `<type>` specifies the type of the variable, and `<name>` specifies the name of the variable.
Furthermore, to ensure thread safety, it is also necessary to specify which thread it is possible to
access the variable from. This is done by the `<thread name>` part. Finally, the variable may be
initialized using the `<initializer>`. The initializer is guaranteed to be executed by the thread to
which the global variable belongs. It typically happens when the variable is accessed the first
time, but is at the discretion of Storm. The initializer may be omitted, in which case the vairable
is constructed using its default constructor.

For example, the code below defines a global string variable, that is only accesible from the
`Compiler` thread.

```bs
Str myString on Compiler = "Hello!";
```

As such, it is *not* possible to access it from the following function:

```bs
void myFn() {
    print(myString); // Error: can only access from the same thread.
}
```

This problem can be addressed by either marking `myFn` as running on the `Compiler` thread:

```bs
void myFn() on Compiler {
    print(myString); // OK, same thread.
}
```

Or by providing an accessor function to the `myString` variable as follows:

```bs
Str getMyString() on Compiler {
    return myString;
}

void myFn() {
    print(getMyString);
}
```