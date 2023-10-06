Functions
=========

A function is defined as follows in Basic Storm:

```
<return type> <name>(<parameters>) {
    <body>
}
```

In the definition, `<return type>` is the name of the return type of the function, `<name>` is the
name of the function, and `<parameters>` is a list of comma-separated parameters to the function.
Each parameter has the form `<type> <name>` where `<type>` is the name of the type, and `<name>`
is the name of the associated parameter. For example, the following code defines a function `pow`
that takes two `Float`s as parameters and returns a `Float`:

```bs
Float pow(Float base, Float exponent) {}
```

Functions that are defined inside a type are considered to be member functions. These functions
will automatically be given a hidden first parameter with the name `this` that contains a
reference to an instance of the class that the function is a member of. This behavior can be
inhibited by marking the function `static` as described below.
