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
a file with the extension `.bs` somewhere on your system. The tutorial uses the name
`fundamentals.bs` for this, but any name that contains only letters works.

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

By default, Storm (and thereby also Basic Storm) assumes that values are always properly
initialized. This is true even for class- and actor types that are handled by reference. As such, it
is *not* possible to store `null` in a `Str` variable, even though the variable is represented as a
pointer internally.

It is, however, useful to be able to represent the absence of a value. One example is the `at`
function in the map container. It needs to be able to both return a value corresponding to the
sought-after key, or a value that indicates that no such value exists. It achieves this by returning
the type `Maybe<V>`, where `V` is the type of the values stored in the map.

The type `Maybe<T>` thereby represent a value that can either be a valid instance of the type `T`,
but it may also contain the special value `null` to indicate that no value of the type `T` is
present. As with arrays and maps, Basic Storm provides the shorthand syntax `T?` that is equivalent
to `Maybe<T>`.

The benefits of using a separate type for nullable values are twofold: first, a special type makes
it very clear both to the compiler and the programmer which values may be null. Secondly, since
nullability is not tied to a particular set of types, it is possible to make any type in the system
nullable. For example, Storm allows `Maybe<Int>` for an integer that may be `null`. However,
languages like Java requires wrapping the primitive `int` in the class `Integer` to make it
nullable.

The first point above is used by Basic Storm to ensure that it is not possible to use a value that
may be null. The type `Maybe<T>` is a completely separate type from `T` in the type system.
Furthermore, `Maybe<T>` provides no way of directly accessing the contained value. It only provides
the members `any` and `empty` that allows checking for the presence or absence of a value. Instead,
Basic Storm provides a special construct for converting a `Maybe<T>` into a `T`. Since this
conversion may fail, it needs to occur inside an if-statement that checks if the conversion was
successful.

To illustrate how this works, let's start by defining a function that attempts to parse a string
into an integer:

```bs
Int? parseInt(Str from) {
    if (from.isInt()) {
        return from.toInt();
    } else {
        return null;
    }
}
```

The idea is that the function returns an `Int` if the conversion was successful, and `null`
otherwise. This is represented by the function returning `Maybe<Int>`, written as `Int?`. From the
example, we can also see that Storm is able to automatically convert values from `Int` into
`Maybe<Int>` (on the line `return from.toInt();`).

We can test so that the function works as intended by defining the following `main` function:

```bs
void main() {
    print(parseInt("10").toS);
    print(parseInt("five").toS);
}
```

This program will print `10` followed by `null` as we expect. However, if we try to use the value
from `parseInt` we will run into problems. For example, let's say we wanted to perform some
computations with the returned `Int?`:

```bs
void main() {
    Int? converted = parseInt("10");
    var modified = converted * 2;
    print(modified.toS);
}
```

If we run the program above we will get the following error:

```
@/home/storm/root/fundamentals.bs(182-183): Syntax error:
Can not find an implementation of the operator * for core.Maybe(core.Int)&, core.Int.
```

The error tells us that Storm has no operator for multiplying a `Maybe<Int>` with an `Int`. This
means that we need to "extract" the `Int` that is inside the `Maybe<Int>` before we can do anything
useful with it. We do this using a [*weak
cast*](md:/Language_Reference/Basic_Storm/Code/Conditionals) as follows:

```bs
void main() {
    Int? converted = parseInt("10");
    if (converted) {
        Int modified = converted * 2;
        print(modified.toS);
    }
}
```

The *weak cast* is a special form of the `if` statement. When the condition is an expression that
evaluates to `Maybe<T>`, the `if` statement will attempt to unwrap the contained `T`. The conversion
is considered successful if the `Maybe<T>` value was not `null`. The `if` statement makes the
extracted `T` available in the "true" branch by declaring a variable that shadows the original one
if possible. The example above is roughly equivalent to the following:

```bsstmt
if (converted.any()) {
    Int converted = converted.extract();
    // ...
}
```

Note that the function `extract` does *not* exist in Storm. It is only present to illustrate what
the weak cast does internally. The important part is that the `if` statement declares a variable wit
the type `Int` that shadows the original one. It thus appears as if `converted` has changed type
inside the body of the `if` statement. An important implication of this is that assigning to
`converted` inside the `if`-statement will only change the local copy, and will not modify the
original variable.

The `if` statement is only able to create a variable that shadows the original one in this manner if
the condition to the `if`-statement is the name of a variable. To illustrate this, assume that we
rewrite our program like this:

```bs
void main() {
    if (parseInt("10")) {
        print("Success!");
        // How to access the result?
    }
}
```

This would work in the sense that the program would print `Success!`. However, it is currently *not*
possible to access the value produced by `parseInt`. For cases like this, it is possible to
explicitly specify a variable name that should contain the result of the conversion. In our case, we
can write like this:

```bs
void main() {
    if (converted = parseInt("10")) {
        Int modified = converted * 2;
        print(modified.toS);
    }
}
```

This is, of course, also possible to do even if the expression to the right of the equal sign is
just a variable.


The weak casts as presented above are enough to work with the `Maybe`-type. However, it gets a bit
cumbersome in certain cases. For example, consider a function `add` that adds two `Maybe<Int>`
together. We can write it as follows:

```bs
Int? add(Int? a, Int? b) {
    if (a) {
        if (b) {
            return a + b;
        }
    }
    return null;
}

void main() {
    Int? result = add(parseInt("10"), parseInt("20"));
    print(result.toS);
}
```

Since it is not possible to use operators like `&` to combine different weak casts, we need to nest
the casts inside each other. This is not too bad in the case of the `add` function. However, it
quickly gets difficult to read if more casts are needed. In situations like this, we would rather
like to write something like this:

```bs
Int? add(Int? a, Int? b) {
    if (!a)
        return null;
    if (!b)
        return null;

    return a + b;
}
```

Unfortunately, this will not work either since there is no `!` operator for the `Maybe<T>` type.
Even if it would provide one, this would mean that the expression in the `if`-statement would
evaluate to a `Bool`, and thus it would no longer be a weak cast.

Fortunately, Basic Storm provides the conditional `unless` that can be thought of as an inverted
`if` statement. In order to work properly with weak casts, the `unless` statement is a bit peculiar
in that it *requires* that the body either returns or throws an exception. This is to guarantee that
the code located after the entire `unless` statement is only executed if the weak cast succeeded.

To illustrate the semantics, consider the `unless` statement below:

```bsstmt:placeholders
unless (<condition>) {
    <if-failed>;
}
<if-successful>;
```

It is equivalent to the following `if`-statement:

```bsstmt:placeholders
if (<condition>) {
    <if-successful>;
} else {
    <if-failed>;
}
```

Using the `unless` statement we can write the `add` function as below. This approach has the benefit
that it remains easy to follow even if multiple weak casts are necessary.

```bs
Int? add(Int? a, Int? b) {
    unless (a)
        return null;
    unless (b)
        return null;

    return a + b;
}
```

Of course it is possible to rename the result variable in the `unless` statement using the equals
sign, just as in the `if` statement. For example:

```bs
Int? add(Int? a, Int? b) {
    unless (unwrappedA = a)
        return null;
    unless (unwrappedB = b)
        return null;

    return unwrappedA = unwrappedB;
}
```

Trailing Returns
----------------

*The code for this part of the tutorial is in the file `root/tutorials/fundamentals/trailing.bs` and
 can be run by typing `tutorials:fundamentals:trailing` in the Basic Storm top-loop.*

Many constructs that are statements in other languages are expressions in Basic Storm. Two examples
of this are blocks and if-statements. This feature was mainly designed to simplify the
implementation of syntax extensions, but it turns out that it is also useful in other situations as
well.

Let us first consider blocks. As in C, C++, and Java, it is possible to introduce new blocks
anywhere inside a Basic Storm function in order to delimit the scope of local variables. For
example, consider the case where we wish to use a string buffer (`StrBuf`) to create a string, but
we do not wish to keep the temporary variable longer than necessary:

```bs
void main() {
    Str string;
    {
        StrBuf tmp;
        for (Nat i = 0; i < 10; i++) {
            if (i > 0)
                tmp << ", ";
            tmp << i;
        }
        string = tmp.toS();
    }
    // "tmp" is no longer accessible.
    print(string.toS);
}
```

One difference from the aforementioned languages is that blocks can also be used as expressions. In
such cases, a block evaluates to the value of the last statement inside the block. We can use this
to make it clearer that the purpose of the block is to initialize the variable `string`. As an
additional bonus, we also avoid creating an empty string to initialize the `string` variable that we
almost immediately discard.

```bs
void main() {
    Str string = {
        StrBuf tmp;
        for (Nat i = 0; i < 10; i++) {
            if (i > 0)
                tmp << ", ";
            tmp << i;
        }
        tmp.toS(); // Last statement, will be the result of the block.
    };
    // "tmp" is no longer accessible.
    print(string.toS);
}
```

Note that it is necessary to add a semicolon after the closing curly brace to terminate the
statement that initializes `string`.

This structure is very convenient when implementing macro-like constructs that need to use temporary
variables. For example, the array initialization syntax expands to a structure similar to the above.
In particular, the literal `Array<Int> x = [1, 2, 3];` expands to the following:

```bsstmt
Array<Int> x = {
    Array<Int> tmp;
    tmp.reserve(3);
    tmp << 1;
    tmp << 2;
    tmp << 3;
    tmp;
};
```

This idea also extends to the top-level block in functions: by default, a function returns the value
of the last statement in the topmost block of the function. This means that it is possible to
implement an `add` function without using `return` like this:

```bs
Int add(Int a, Int b) { a + b; }
```

Finally, the same idea also extends to `if` statements. For example, we could rewrite the
`if`-statement in the loop above as follows:

```bs
void main() {
    Str string = {
        StrBuf tmp;
        for (Nat i = 0; i < 10; i++) {
            tmp << if (i > 0) { ", "; } else { ""; };
            tmp << i;
        }
        tmp.toS(); // Last statement, will be the result of the block.
    };
    // "tmp" is no longer accessible.
    print(string.toS);
}
```

Since the `if` statement can be used as an expression, Basic Storm does not need a dedicated ternary
operator like C, C++, and Java. While using the `if`-statement as an expression did perhaps not
improve the readability in the example above, this ability can be quite useful when working with
`Maybe` types. For example, consider a case where we accept a `Maybe<Nat>` as a parameter to a
function. If the supplied value was not `null`, we wish to use the value. Otherwise, we wish to use
some default value. We can implement this compactly as follows:

```bs
Nat computeStuff(Nat? input) {
    Nat input = if (input) {
        input;
    } else {
        0;
    };
    // Our computations go here:
    input + 1;
}
```

