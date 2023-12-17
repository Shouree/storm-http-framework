Fundamentals in Basic Storm
===========================

This tutorial introduces and illustrates the fundamentals of Basic Storm. The tutorial is not aimed
at teaching programming, and it therefore assumes familiarity with programming. Ideally in
imperative languages like C, C++, and Java.

The code produced in this tutorial is available in the directory `root/tutorials/fundamentals` in
the release of Storm. You can run it by typing `tutorials:grammar:<step>` in the Basic Storm
interactive top-loop. Replace `<step>` with the name of the step for the relevant caption. Note that
the code in the distribution differs slightly from what is presented here to allow the different
steps to coexist in the same package. The only difference is that the name of the function `main`
has been changed.

Setup
-----

Before starting to write code, we need somewhere to work. For this tutorial, it is enough to create
a file with the extension `.bs` somewhere in your system. The tutorial uses the name
`fundamentals.bs` for this, but any name that contain only letters works.

When you have created a file, open a terminal and change to the directory where you created the
file. You can instruct Storm to run the file by typing:

```
storm fundamentals.bs
```

Note that based on how you have installed Storm, you might need to [modify the
command-line](md:../Running_Storm/In_the_Terminal) slightly. Also, if you named your file something
different than `fundamentals.bs`, you need to modify the name in the command accordingly.

If you have done everything correctly, the command will currently not appear to do anything since
the file did not contain a syntax error, nor a function named `main`. You can exit the top-loop by
typing `exit` and pressing Return. If you see a line starting with `WARNING:`, however, you have
most likely specified the wrong name of the file, or have changed into the wrong directory in your
terminal. Storm will print the full path of the file it attempts to open in the warning to aid in
debugging.

To verify that Storm is examining the right file, we can simply define a `main` function as follows
and try again:

```bs
void main() {
}
```

If done correctly, Storm will now instead stay silent and exit almost immediately. This is the
starting point we wish to have for the remaining steps in the tutorial.

Outputting Text
---------------

*The code for this part of the tutorial is in the file `root/tutorials/fundamentals/text.bs` and can
 be run by typing `tutorials:fundamentals:text` in the Basic Storm top-loop.*

A good first tool when working with a new programming language is the ability to print things to
standard output. This lets us inspect and test a great number of things, from printing the results
of computations to verify our understanding of the language, to printing traces that lets us debug
the behavior of programs.

In Storm, this is most conveniently done with the `print` function. The full name of the `print`
function is [stormname:core.print(core.Str)]. From the name, we can deduce that the `print` function
is located in the package named `core`, and that it accepts a single `core.Str` (i.e. a string) as a
parameter. Luckily for us, Basic Storm and many other languages import the `core` package for us
automatically, so we can use it right away.

We can call the `print` function from the `main` function we created above as follows as a first
step:

```bs
void main() {
    print("My first program in Basic Storm!");
}
```

If we run the program now (using `storm fundamentals.bs`), we will see that our message is being
printed as we would expect:

```
My first program in Basic Storm!
```

Let's try printing something more interesting. For starters, let's just write a list of numbers
from 0 to 9 using a for-loop:

```bs
void main() {
    for (Nat i = 0; i < 10; i++) {
        print(i);
    }
}
```

From the code above, we can immediately note two things. First, that Storm (and thereby Basic Storm)
uses the convention that *all type names start with an uppercase letter*. From this, we can deduce
that `Nat` is the name of a type. It is the name of the unsigned integer type (for *natural number*)
in Storm. We could use `Int` as well if we wished, but Storm tends to use unsigned types wherever it
is suitable.

Apart from these two observations, the code above looks fairly similar to other C-style languages
such as C++ and Java. However, if we try to run it, we get the following error:

```
@/home/storm/fundamentals.bs(57-62): Syntax error:
Can not find "print" with parameters (core.Nat&).
```

This message tells us that Basic Storm could not find a version of the `print` function that accepts
a `Nat` as a parameter. It turns out that Storm only provides an overload that accepts strings. This
means that we need to convert the number into a string. Luckily, nearly all types in Storm provides
a function called `toS()` (short for "to string") that creates a string representation of objects.
We can now update our program to fix the error:

```bs
void main() {
    for (Nat i = 0; i < 10; i++) {
        print(i.toS());
    }
}
```

Note that Basic Storm makes no difference between empty parentheses and no parentheses at all. This
means that it is possible to simply write `print(i.toS)` if desired. This is useful when refactoring
code, as it allows turning member variables into member functions without changing other code.

If we run the program again, we see the output we would expect:

```
0
1
2
3
4
5
6
7
8
9
```

Let's continue with our program and have it compute the Fibonacci numbers:

```bs
void main() {
    Nat current = 0;
    Nat next = 1;
    for (Nat i = 0; i < 10; i++) {
        print("fib(" + i.toS() + ") is: " + current.toS());
        Nat sum = current + next;
        current = next;
        next = sum;
    }
}
```

We can note that Storm overloads the `+` operator to allow string concatenation. Just as with the
`print` function, we must first convert any non-strings into strings using the `toS` function. As
expected, we get the following output:

```
fib(0) is: 0
fib(1) is: 1
fib(2) is: 1
fib(3) is: 2
fib(4) is: 3
fib(5) is: 5
fib(6) is: 8
fib(7) is: 13
fib(8) is: 21
fib(9) is: 34
```

As we can see, this quickly gets rather inconvenient (and inefficient, since each `+` needs to
create a new string). One option is to use *interpolated strings* to quickly and efficiently create
the desired representation. In contrast to some other languages, interpolated strings in Basic
Storms are resolved entirely at compile time, and compile into code that uses the
[stormname:core.StrBuf] class (short for "string buffer") to efficiently build the string
representation. It also helps us with converting values into strings automatically where possible.
It is therefore very convenient during debugging. In our code, we can use it as follows:

```bs
void main() {
    Nat current = 0;
    Nat next = 1;
    for (Nat i = 0; i < 10; i++) {
        print("fib(${i}) is: ${current}");
        Nat sum = current + next;
        current = next;
        next = sum;
    }
}
```

As we can see, interpolated strings allows us to include a `$`-sign followed by curly braces.
Whatever is inside of the curly braces is an expression that is evaluated and inserted at that
position in the string. As noted before, this is done during compilation, so you will get a
compile-time error if you have mistyped a variable name for example.

The interpolated string syntax also allows us to format our table nicely. We can specify formatting
options for an inserted value by adding a comma (`,`) followed by formatting options. For example,
we can set the width of a field by specifying a number, and right-align the numbers by specifying
`r`. In our case, it may look like this:

```bsstmt
print("fib(${i2r}) is: ${current,5r}");
```

A full reference of the formatting options available in interpolated strings is available
[here](md:/Language_Reference/Basic_Storm/Code/Literals).


Values and Classes
------------------

Actors are covered in the next part.

- By value vs by reference
- Adding custom types to the `toS` mechanism
- Uniform Call Syntax
- parens vs. no parens (for refactoring)



Containers
----------

- Also: loops

Command-Line Arguments
----------------------

- `argv`
- return from `main`

Stack Traces
------------

