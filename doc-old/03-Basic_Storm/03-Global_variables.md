Global variables
=================

Global variables are declared outside of types and functions. Declaring a global variable follows
the same syntax as variables inside functions and inside types. However, since global variables are
associated with a thread in Storm, it is necessary to specify a thread for each global variable,
like so:

```
Int global on Compiler;
```

As noted [here](md://Storm/Type_system), this means that global variables are only accessible to the
specified thread. Thus, the following example will not compile:

```
Int global on Compiler;

Int foo() {
    return global + 10;
}
```

There are two possible ways of solving the problem: either declare `foo` to execute on the
`Compiler` thread as well, or provide an accessor to the global variable that executes on
`Compiler`. The reason for Storm being strict in this case is that global variables usually need
some kind of synchronization (e.g. initialize the first time only, keeping multiple variables up to
date). Enforcing access only from a single thread means that Storm encourages writing accessors that
handle the synchronization properly, hopefully avoiding many cases of missing synchronization.


Initialization
---------------

If no explicit initialization is specified, Basic Storm attempts to default initialize all global
variables. If this is not possible, or if another default value is desired, it is possible to
explicitly initialize the global variables like so:

```
Int global on Compiler = 10;
```

Initialization expressions are executed on the specified thread when the variables are inserted in
the name tree (i.e. when Storm first loads the code in the file), and may be arbitrarily complex.
