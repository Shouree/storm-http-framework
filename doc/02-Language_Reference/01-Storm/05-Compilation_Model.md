Compilation Model
=================

Storm is a compiled language. This means that all source code is compiled down to machine code
before it is executed. The compilation process is managed automatically by Storm, so no separate
compilation step is necessary.

Storm utilizes *lazy compilation* to guide its compilation process. At a high level, this means that
Storm strives to do a minimal amount of compilation work to provide the information required by
other parts of the system. As such, there is no distinct concept of *compile-time* in Storm, as
compilation is interleaved with execution of the program. For example, when a language attempts to
find a type `T` in a package, Storm needs to parse the relevant source files to find the available
types. This happens when the system requests the information for the first time. It will, however,
not populate types with member variables or function calls until that information is requested.
Similarly, this work is performed whenever the information is requested by other parts of the
system. Finally, functions are generally not compiled into machine code until they are called for
the first time.

There are two main benefits of utilizing *lazy compilation* in this way. First, it reduces the
start-up time of the system, since the entire system does not need to be compiled ahead of time (as
of currently, Storm is not able to store compiled code to disk). Secondly, it allows running a
program that contains errors, as long as the parts containing errors are not executed. This is
beneficial during program development, as it allows testing early prototypes without having to
address all issues up front.

The last point might come as a surprise, since most languages in Storm are strongly typed. The fact
that Storm is generally strongly typed seems to imply that it would not be possible to run a program
that contains errors. While this is true for languages that are compiled ahead of time, the lazy
compilation model in Storm means that such errors are only found and reported when the relevant
portion of the program is actually compiled. In general, this means that different errors are
reported at different stages of program execution:

* **Syntax errors**: Syntactical errors, when the source code does not conform to the grammar of the
  used language, are generally reported very early in program execution. Parsing the source text
  into an abstract syntax tree is typically one of the first things that languages in Storm perform,
  and as such this often happens as soon as a package is used.

* **Type- and function definition errors**: Errors in type- and function definitions, for example,
  the type of a function parameter does not exist, or that a member variable in a type does not
  exist. For functions in the global scope, these occurr whenever Storm needs to determine if a type
  or function exists or not, so generally as soon as some part of the system uses some part of the
  relevant package. For types, this happens whenever Storm needs to examine the contents of the
  type. Typically, this only happens when a function that uses the type is compiled.

* **Semantic errors in functions**: The last category of errors, semantic errors inside functions,
  are only discovered and reported when the function body is compiled. This typically happens when
  the function is called for the first time. This means that it is possible for functions that have
  never been called to contain semantic errors.

As we can see from above, the lazy compilation model means that it is not enough to start a program
to ensure that it is free from compile-time errors. For example, creating a main function that
simply prints `hello world` does not ensure that other functions in the same file are free from
semantic errors, or that the types in the package are properly specified. This approach only ensures
that the source file(s) are syntactically correct.

Because of this, it is typically beneficial to take a test-driven approach when developing programs
in Storm. The tests do not have to be very rigorous as they often have to be in dynamic languages.
It is enough that the test ensures that relevant functions are called, and that the relevant types
are used. This will ensure that types and functions are compiled, and that any semantic errors are
reported.

At certain points during program development, for example before releasing the program, it is useful
to tell Storm to compile all code and report all errors. This can be achieved by calling the
`compile` function on any named object in the system. This ensures that the object and any
descendants in the name tree are fully compiled by the time that `compile` returns. In Basic Storm,
this can be achieved as follows:

```bs
use lang:bs:macro; // Needed for the "named" macro.

void main() {
    // Ensures that "my:package" is compiled:
    named{my:package}.compile();
    // Ensures that the current package is compiled:
    named{}.compile();
}
```