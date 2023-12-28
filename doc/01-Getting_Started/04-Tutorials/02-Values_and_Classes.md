Values and Classes
==================

Storm, and therefore also Basic Storm, has three kinds of types: value types, class types, and actor
types. This tutorial cover the first two of them: value types and class types. In particular, we
will look at the differences between value types and class types, and how to implement new types in
Basic Storm.

The code presented in this tutorial is available in the directory `root/tutorials/types` in the
release of Storm. You can run it by typing `tutorials:types:main` in the Basic Storm interactive
top-loop.

Setup
-----

Before starting to write code, we need somewhere to work. For this tutorial, it is enough to create
a file with the extension `.bs` somewhere on your system. The tutorial uses the name `types.bs`,
but any name that contains only letters works. As a starting point, add the following code to the
file:

```bs
void main() {
}
```

When you have created a file, open a terminal and change to the directory where you created the
file. You can instruct Storm to run the file by typing:

```
storm types.bs
```

Note that based on how you have installed Storm, you might need to [modify the
command-line](md:../Running_Storm/In_the_Terminal) slightly. Also, if you named your file something
different than `types.bs`, you need to modify the name in the command accordingly.

If done correctly, Storm will find your file, notice that it contains a `main` function and execute
it. Since the `main` function does not yet do anything, Storm will immediately exit without printing
anything.


Values
------

- Note on inheritance.


Classes
-------


Function Call Syntax
--------------------

- Note that we don't need to use parens.
- Use this to refactor the class with assign functions!


Uniform Call Syntax
-------------------

- Extend a class from elsewhere (e.g. the string class)
- Note that we can use `this` as a parameter name.




Actors are covered in the next part.

- By value vs by reference
- Adding custom types to the `toS` mechanism
- Uniform Call Syntax
- parens vs. no parens (for refactoring)
