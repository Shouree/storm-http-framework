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
print("fib(${i,2}) is: ${current,5}");
```

A full reference of the formatting options available in interpolated strings is available
[here](md:/Language_Reference/Basic_Storm/Code/Literals).


Containers
----------

Storm's [standard library](md:/Library_Reference/Standard_Library/Containers) contains a number of
useful containers. These containers are *generic* which means that they can store any type in the
system. The type is specified as a parameter to the type, much like templates in C++ or generics in
Java.

### Arrays

*The code for this part of the tutorial is in the file `root/tutorials/fundamentals/containers.bs`
 and can be run by typing `tutorials:fundamentals:arrays` in the Basic Storm top-loop.*

The most fundamental container is perhaps the `Array<T>`. It is an array of elements of type
`T`. Basic Storm provides a shorthand for the type as `T[]`. Elements in arrays are accessed using
the `[]` operator as in many other languages. New elements can be added with the `push` function, or
with the `<<` operator. With this in mind, we can rewrite the previous example to compute the first
few Fibonacci numbers and store them in an array:

```bs
void main() {
    Nat[] numbers = [0, 1];
    for (Nat i = 2; i < 15; i++) {
        numbers << numbers[i - 1] + numbers[i - 2];
    }
    print(numbers.toS());
}
```

The first line in the program uses an array literal to create the initial array. This syntax
attempts to automatically deduce the array type based on the elements and the context. If the
automatic deduction fails, it is possible to specify the desired type as follows:

```bsstmt
Nat[] numbers = Nat:[0, 1];
```

This is usually only necessary to do when working with empty arrays and/or overloaded functions.

In the loop, we then use the `<<` operator to add elements to the array, and the `[]` operator to
access elements. Finally, we use the `toS` function to generate a string representation.

If we wish to format the output a bit nicer, we can iterate through the array using a regular
for-loop as follows:

```bsstmt
for (Nat i = 0; i < numbers.count; i++) {
    print("${i,2}: ${numbers[i],3}");
}
```

Storm does, however, also provide a loop construct that is specificially designed to iterate through
elements in containers. It is semantically equivalent to the code above, but utilizes
[iterators](md:/Library_Reference/Standard_Library/Iterators) if possible. With it, we can instead
write:

```bsstmt
for (k, v in numbers) {
    print("${k,2}: ${v,3}");
}
```

In the loop above, we provide two iteration variables named `k` and `v`. The first one is optional,
and is used for the "key" for the container we iterate through. For arrays, the key is the index of
the element. The second is the "value" for the container. This is naturally the element itself for
arrays. As we shall see, this scheme naturally extends to maps as well.

Since the key is optional, we could simply write the following if we were only interested in the
value itself:

```bsstmt
for (v in numbers) { /* ... */ }
```

Finally, Storm provides a function called `join` that is useful for concating arrays of elements
into strings. For example, we can create a representation that separates the computed Fibonacci
numbers with arrows as follows:

```bsstmt
print(join(numbers, " -> ").toS());
```

By default, the `join` function uses the standard to-string implementation. It is, however, possible
to provide a lambda function that specifies how to convert elements into strings before
concatenating them.


### Maps

*The code for this part of the tutorial is in the file `root/tutorials/fundamentals/containers.bs`
 and can be run by typing `tutorials:fundamentals:maps` in the Basic Storm top-loop.*

Another useful container is the `Map<K, V>` type. Similarly to arrays, Basic Storm provides a
shorthand syntax for the `Map<K, V>` type as: `K->V`. A map contains an set of pairs. Each pair
contains a *key* (of type `K`) and a *value* (of type `V`). The map indexes the keys so that lookups
based on keys are efficient. In contrast to the `Array<T>`, however, the `Map<K, V>` is unordered.
This means that while it is possible to iterate through the elements in a map, there is no
guarantees that the order in which elements appear are the same as the one they were inserted in.
Furthermore, the order may even *change* between different iterations, even if the map has not been
modified.

Elements are inserted into the map with the `put` function as follows:

```bs
void main() {
    Nat->Str numbers;
    numbers.put("zero", 0);
    numbers.put("one", 1);
    numbers.put("two", 2);
    numbers.put("three", 3);
    numbers.put("ten", 10);

    print(numbers.toS);
}
```

By running the above example, we can clearly observe that the order of elements are not preserved.
The `toS` operator iterates through the elements when creating the string representation, and places
them in the following order, that is seemingly arbitrary:

```
{three -> 3, one -> 1, ten -> 10, zero -> 0, two -> 2}
```

The map requires that keys are unique, so if the key passed to `put` already exists, then `put` will
update the existing pair. The `put` function actually returns a boolean that indicates if it
inserted a new pair or not. If `put` returns true, then a new pair was inserted. Otherwise, an
existing pair was updated.


We can iterate through the map manually using the `for` loop as with arrays as follows:

```bsstmt
for (k, v in numbers) {
    print("${k,5l} -> ${v,2}");
}
```

If the keys are not desired for some reason, it is possible to omit the `k` variable, as with
arrays.

The map type also provides a number of ways to look up and modify the elements in the array. The
simplest of them is `has` that checks if a key is present in the map:

```bsstmt
numbers.has("one"); // -> true
numbers.has("twenty"); // -> false
```

We can then get the value associated with a key using the `get` function. It retrieves the value
associated with a key, and throws an exception if the it does not exist. For example:

```bsstmt
print(numbers.get("one")); // -> 1
print(numbers.get("twenty")); // -> throws MapError
```

It is also possible to provide a default element to the `get` function that will be returned in case
the requested key does not exist. Note that the default value is *not* inserted into the map:

```bsstmt
print(numbers.get("one", 0)); // -> 1
print(numbers.get("twenty", 0)); // -> 0
```

Note, however, that since `get` is a normal function, the default value is always created, even if
it is not needed. This means that it is probably not a good idea to create a large object and pass
it as the default value to `get`. This means that the large object is always created, even if it is
not used. For such cases, it is likely better to use `at` (see below).

Whenever the type `V` has a default constructor, the map also provides a `[]` operator that allows
treating it like an array. The `[]` operator works like in C++ in that it never throws. Rather, if
the requested key was not found, a default-constructed value is inserted in the map, and then a
reference to that value is returned. To illustrate the behavior, we can write as follows:

```bsstmt
print(numbers["one"]); // -> 1
print(numbers["twenty"]); // -> inserts "twenty" -> Nat(), returns 0
```

This might seem strange at first. It is, however, quite useful in many situations. For example, if
`words` is an array of strings that each contain a word, we can compute a word frequency table as
follows:

```bs
Str->Nat wordFrequency(Str[] words) {
    Str->Nat result;
    for (word in words) {
        result[word] += 1;
    }
    return result;
}
```

The final function for accessing elements is the `at` function. It attempts to make it convenient to
look up a key and extracting the value, while taking into account the fact that the key may not
exist. One way to do this is to use the `has` and `get` functions as follows:

```bsstmt
if (numbers.has("twenty")) {
    return numbers.get("twenty");
} else {
    return 0;
}
```

This does, of course, work as intended. It does, however, require two searches in the map. The map
is implemented as a hash table, so lookups are fairly cheap, but they are far from free. The `at`
function solves this by looking up the key, and returning the value as a `Maybe<V>`, to represent
the fact that the value may not exist. This means that we can implement our lookup as follows:

```bsstmt
if (value = numbers.at("twenty")) {
    return value;
} else {
    return 0;
}
```

To fully understand why this works, we need to understand `Maybe<T>` types. They are introduced in
the section below this.

Before we move on to maybe types, it is worth mentioning some details regarding how the map handles
keys. In general, the hash table uses the same concept of equality as the `==` operator does. That
is, value- and class-types are compared by-value, while actor types are compared by-reference.

As mentioned previously, the map is implemented as a hash table. This means that the `Map<K, V>`
requires that the type `K` has a `hash()` function and a comparison operator. Most primitive types
in Storm implement this functionality, but it is not generated automatically for custom types. As
such, to use custom types as keys in a map, it is necessary to implement a `hash` function (that
returns a `Nat`), and a comparison function (a function named `==`).


Maybe
-----

*The code for this part of the tutorial is in the file `root/tutorials/fundamentals/maybe.bs`
 and can be run by typing `tutorials:fundamentals:maybe` in the Basic Storm top-loop.*

As we saw above, the `at` function in the map type returned a value of the type `Maybe<T>`. By
default, Storm assumes that values, even for class- and actor-types are not `null`. It is, however,
often useful to have a variable that may either contain some value, or be "empty". This scenario is
represented by the type `Maybe<T>`. A value of the type `Maybe<T>` may either contain a value of the
type `T`, or the special value `null` that represent the absence of a value.

What makes the type `Maybe<T>` a bit unique is that while the type provides the functions `any` and
`empty` to check if a value is present, it does not provide a way to access the value. Rather, a
*weak cast* must be used to extract the value inside a condition that checks for the presence of the
value. This means that it is not possible to accidentally access a `null` value and crash.

...



Command-Line Arguments
----------------------

- `argv`
- return from `main`

Stack Traces
------------

Trailing Returns?
-----------------