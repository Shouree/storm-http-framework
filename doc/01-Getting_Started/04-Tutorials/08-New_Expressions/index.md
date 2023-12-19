New Expressions and Statements in Basic Storm
=============================================

This part of the tutorial shows how to create syntax extensions to Basic Storm. We will implement
two relatively simple extensions. The first one is a loop-like repeat block that simply executes the
loop body a certain number of times:

```bsstmt:use=tutorials.expressions.ast
repeat(10) {
    print("Test");
}
```

The second one is a simple map-like construct (limited to integers for simplicity):

```bsstmt:use=tutorials.expressions.ast
Int[] input = [1, 2, 3];
Int[] output = map(x * 2 for x in input);
```

There are a number of different ways in which these two extensions can be implemented. This tutorial
illustrates two of them. The first, and perhaps easiest, is to implement these new constructs in
terms of Basic Storm and thereby rely on Basic Storm's ability to generate code. The second one,
that might be a slightly more involved, is to generate code in the Intermediate Language directly.
Since this approach does not rely on Basic Storm, it is not limited by the abilities of Basic Storm.

As with the other tutorials, the code produced by this tutorial is available in
`root/tutorials/expressions`. You can run it by typing `tutorials:expressions:main` in the Basic
Storm top loop. Since we are implementing two versions of the same constructs, the structure of the
aforementioned directory differs slightly from the one suggested by the tutorial. Namely, the two
implementations are in different subdirectories so that they can coexist, and so that it is possible
to decide which one to use.

Setup
-----

First, we need somewhere to work. Create an empty directory somewhere on your system. The name does
not matter too much, but the remainder of the tutorial assumes that it is `expressions`. If you pick
another name, you need to modify the package names in `use` statements.

Create two subdirectories inside the directory you just created: `repeat` and `map`. We will place
the two extensions inside these directories. This is so that we can create a `main` function that
uses the extensions in a separate package from the extension itself. It is usually not a good idea
to define and use a language extension in the same package, as that easily creates dependency cycles
that are difficult to resolve.

Finally, create a file named `test.bs` (or some other name) inside the `expressions` directory. Start
by adding a definition of a `main` function as below:

```bs
void main() {
}
```

We will use this function to write tests later on. After all of this, open a terminal and navigate
to the newly created directory. This makes it possible to run the code we will write by typing:

```
storm .
```

If done successfully, this will not produce any output, and Storm will exit successfully.


Tutorials
---------

To help with readability, this tutorial is split into four pieces:

- [`repeat` using Basic Storm's AST](md:Repeat_Using_AST)

- [`repeat` using the Intermediate Language](md:Repeat_Using_IR)

- [`map` using Basic Storm's AST](md:Map_Using_AST)

- [`map` using the Intermediate Language](md:Map_Using_IR)
