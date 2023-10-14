Comments
========

There are two ways to write a comment in Basic Storm. Single-line and multi-line comments. The
single-line comment starts with `//` and ends at the end of the line. The multi-line comment starts
with `/*` and ends with `*/`. For example:

```bsstmt
code; // This is a comment.
complex /* this is a comment */ code;
```

In contrast to other languages (e.g. C and C++), it *is* possible to nest multiline comments:

```bsstmt
code
/* multiline comments
   /* can */
   be nested
 */
code;
```

Comments that are placed before a declaration in Basic Storm are treated as documentation. They do
not affect the behavior of the program, but the system stores them (or rather, their location) so
that it is possible to view it using the `help` command in the Basic Storm top loop or using the
language server.

Comments aimed at documentation do not need to have any special form, but they are usually formatted
like below for classes and functions. For the cases of classes, the system properly strips the
leading asterisks (`*`) before showing the documentation text.

```bs
/**
 * My class.
 *
 * It performs the following functions:
 * - ...
 */
class MyClass {
    // ...
}

// My function. It is a helper to create MyClass.
MyClass createMyClass() {
    // ...
}
```
