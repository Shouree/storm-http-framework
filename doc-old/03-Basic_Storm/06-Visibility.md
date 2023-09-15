Visibility
===========

Basic Storm allows specifying the visibility of declarations at the global level and inside
types. The syntax used is a hybrid between C++ and Java. It is possible to specify visibility for
individual declarations by including the visibility first, like so:

```
public Int foo() { 3; }
```

It is also possible to declare the visibility for a group of functions like in C++:

```
public:

Int foo() { 3; }
Int bar() { 4; }
```

If no visibility is specified at all, Storm defaults to `public`.

Basic Storm supports the following types of visibility for declarations outside of types:

* `public` visible from everywhere in the system.
* `package` visible from the current package and any sub-packages.
* `private` visible only from the current file.

The following types of visibility can be used inside types (both classes and values):

* `public` visible from everywhere in the system.
* `protected` visible from the current class and sub-classes.
* `package` visible from the current package and any sub-packages.
* `private` visible only from the current type.

