Global Variables
================

A global variable is defined as follows:

```
<type> <name> on <thread name> [= <initializer>];
```

Where `<type>` specifies the type of the variable, and `<name>` specifies the name of the
variable. Furthermore, to ensure thread safety, it is also necessary to specify which thread it is
possible to access the variable from. This is done by the `<thread name>` part. Finally, the
variable may be initialized using the `<initializer>` to provide an expression that is evaluated
when the variable is first accessed. The initializer may be omitted, in which case the vairable is
constructed using its default constructor.

For example, the code below defines a global string variable, that is only accesible from the
`Compiler` thread.

```bs
Str myString on Compiler = "Hello!";
```
