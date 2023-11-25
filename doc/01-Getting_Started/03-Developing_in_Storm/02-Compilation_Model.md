Compilation Model
=================

One thing that is unusual for Storm is that it utilizes lazy compilation to only compile the parts
of the program that needs to be executed. This means that some errors are not reported immediately,
contrary to what happens in most other compiled languages.

The most important implication of this is that it is not possible to conclude that an entire program
compiles without errors just because it starts. Rather, it is necessary to either ask the system to
compile the entire program, or to ensure that all functions have been called at least once. It is,
however, not necessary to provide an extensive test suite to ensure correctness (like in some
interpreted languages), since most languages in Storm performs type-checking on the entire function
as soon as it is compiled.

The positive side of this is that Storm is able to start running a program faster than if it had to
compile all functions up front. Another benefit is that it is possible to test parts of a program
that contains errors, as long as the erroneous parts are never executed. This is sometimes
beneficial when prototyping changes to a program, or during early development.

The Problem
-----------

To illustrate the problem, consider the following program in the file `demo.bs`:

```bs:partial
void makeBox(Str) {
    print("-" * x);
    print(x);
    print("-" * x);
}

void greeting(Str name) {
    print("Hello " + name);
}

void main() {
    greeting("Test");
}
```

If we run this program using `storm demo.bs` we immediately get the following error:

```
@/home/user/storm/demo.bs(16-17): Syntax error: Unexpected ')'. Expected:
  ":"
  lang.bs.SDelimiter'
  lang.bs.SName
  "[ \n\r\t]*"
  lang.bs.SType'
  "<"
  "->"
  "[A-Za-z_][A-Za-z0-9_]*"
  "/"
  "\[\]"
  lang.bs.SDelimiter
  "&"
  lang.bs.SCommentStart
  "\?"
  "//[^\n\r]*[\n\r]"
```

This is an error that indicates that our code did not parse successfully. Looking up character 16
points us to the line: `void makeBox(Str)`. As we can see, we have forgotten to add a variable name
here. If we fix the code by naming the parameter `x`, we get the following code:

```bs
void makeBox(Str x) {
    print("-" * x);
    print(x);
    print("-" * x);
}

void greeting(Str name) {
    print("Hello " + name);
}

void main() {
    greeting("Test");
}
```

If we now run the program (using `storm demo.bs`) we get no errors, and the program appears to work
as expected, as it prints `Hello Test`. The function `makeBox` does, however, contain a type error
that will be reported when the function is executed. We can see this by launching the top-loop and
calling the function manually:

```
storm -i demo.bs
Welcome to the Storm compiler!
Root directory: /home/filip/Projects/storm/root
Compiler boot in 59.95 ms
Type 'licenses' to show licenses for all currently used modules.
bs> demo:makeBox("Test")
@/home/user/storm/demo.bs(36-37): Syntax error: Can not find
  an implementation of the operator * for core.Str, core.Str&.
```

Why was the first error reported immediately, and not the second? To understand this, we need to
understand roughly what Storm needs to do in different stages. When Storm looks for the `main`
function, it has to first parse all files in the package (only `demo.bs` in this case) in order to
see if there is a function named `main` or not. The first error we saw happened during this stage:
Storm failed to parse the text in the file, which it had to do to find `main`.

The second error, however, was not related to a parsing error. The file parsed successfully, which
allowed Storm to conclude that there was indeed a `main` function. As such, Storm did not need to
compile the program further at that time. Since `main` only calls `greeting`, storm only ever needs
to examine those two functions in detail. Since Storm never had to execute `makeBox`, it was never
examined, and therefore the error was never found.


Ensuring All Errors are Reported
--------------------------------

There are two ways to force Storm to detect and report the error. The first is simply to make sure
that the function is called at some point. For example, we could add a call to `makeBox` inside the
`main` function like this:

```bs
void main() {
    greeting("Test");
    makeBox("Test");
}
```

As mentioned before, the test does not have to be exhaustive. It is enough to ensure that all
functions are called. Note, however, that the following would not be enough, since the function is
not called:

```bs
void main() {
    greeting("Test");
    if (false) {
        makeBox("Test");
    }
}
```

Note: The reason is *not* that Storm is able to determine that the body of the is statement is
meaningless. The same would be true even if the condition was more complex.


The second option is to explicitly ask Storm to compile our code ahead of time. This can be done by
calling the `compile` member on the relevant named entities. The named entities can be accessed
using the `named` syntax like below:

```bs
use lang:bs:macro;

void makeBox(Str x) {
    print("-" * x);
    print(x);
    print("-" * x);
}

void greeting(Str name) {
    print("Hello " + name);
}

void main() {
    named{}.compile(); // named{} gets the current package
    greeting("Test");
}
```

The `compile` function recursively compiles whichever entity it is called on. In this case we ask
Storm to compile everything below the current package, which includes the problematic function. Note
that this means that the error is reported whenever the `compile` function is called, but not before
that.


Corrected Code
--------------

For completeness, here is the fixed source code:

```bs
use lang:bs:macro;

void makeBox(Str x) {
    print("-" * x.count);
    print(x);
    print("-" * x.count);
}

void greeting(Str name) {
     print("Hello " + name);
}

void main() {
	named{}.compile();
	greeting("Test");
}
```
