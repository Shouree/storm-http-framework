Types
=====

The syntax for type definitions are similar for values, classes, and actors. Defining a value type
is done using the `value` keyword as follows:

```
value <name> [: <options>] {
    <members>
}
```

The syntax above defines a value named `<name>`. `<members>` is a list of definitions of the members
of the value. Refer to [definitions](md:) for allowed definitions in types. Options will be
discussed further below.

A class is defined using the `class` keyword:
  
```
class <name> [: <options>] {
    <members>
}
```

Similarly to the definition of a value, `<name>` indicates the name of the class, and `<members>`
is a sequence of members of the class.

Actors are also defined using the `class` keyword as above. A class is an actor if either the `on
<thread>` option is present, or if it inherits from an actor.



Members
-------

Types may contain definitions of [functions](md:Functions) (that become member functions),
constructors, and member variables. The two latter may only appear inside types, and are described
below.

Similarly to top-level definitions, it is possible to specify visibility of members either by
prepending the visibility modifier to each member, or by specifying them for many members at once
(e.g. `private:`).


Member Variables
----------------

A member variable is defined as follows:

```
<type> <name> [= <initializer>];
```

Here, <type> is the name of the type of the variable, and `<name>` is the name of the variable. An
initializer may also be specified. If specified, the initializer is used to initialize the variable
if it is not explicitly initialized to some other value in the `init`-block in a constructor. If no
initializer is specified, and no value is present in the `init` block, the default constructor is
used if it exists.


Constructors
------------

Basic Storm attempts to ensure that it is not possible to access uninitialized variables. Because of
this, constructors are treated specially in Basic Storm. They are defined using a special syntax as
follows:

```
init(<parameters>) {}
```

Similarly to function declarations, `<parameters>` is a list of comma-separated parameters to the
constructor. If the constructor is inside an actor that requires a thread to its constructor, the
first parameter must be of the type `Thread`.

What makes constructors special is that they need to contain an `init` block. The `init` block
represents actually creating the instance by initializing all member variables. An initialization
block has the following form in general:

```
init[(<parameters>)] {
    <name> = <initializer>;
    <name>(<ctor-parameters>);
    ...
}
```

The first part, the optional parenthesis with parameters, is used to pass any required parameters to
the constructor of the parent class. If no parameters are required, or if parameters have already
been passed earlier, this part can be omitted.

The next part contains a list of all variables that should be initialized. These either have the
form `<name> = <initializer>;`, which creates the variable by copying the result from an expression,
or the form `<name>(<ctor-params>);`, which creates the variable by calling the appropriate
constructor. Any variables that are not mentioned in the list are initialized using the initializer
specified where the member variable was declared. If no initializer was specified, the default
constructor is used if it exists, otherwise an error is raised.

It is worth noting that the order in which variables are initialized does not necessarily match the
order in which the variables are listed in the `init` block. Rather, the system initializes them in
the order they appear in memory.


Since the `init` block is special since it represents the creation of the object itself. As such,
the `this` variable is only accessible after the `init` block has been executed. This is best
illustrated with an example:

```bs
class MyClass {
    Int a;
    Int b;

    init(Int b) {
        // Here, 'this' does not exist. We can therefore not access any members.
        a = 10; // Invalid, no instance yet.

        init {
            a = 1;
            // This works as expected. The left hand side has to refer to a
            // member variable. The right hand side can not refer to a member
            // variable, as we don't yet have an instance. It must therefore
            // refer to the parameter.
            b = b;
        }

        // Now we can access the members:
        a = 10; // Valid, we have an instance.
    }
}
```

In certain situations it might be useful to create the object in two stages. First, call the
constructor of a super class, and then initialize the parts of the derived class. This can be done
by explicitly calling `super(<parameters>)` before the `init` block. If this is done, no parameters
may be provided to `init`, as the parent class' constructor has already been called. This can be
illustrated by the following class, that inherits from `MyClass` above:

```bs
class MyDerivedClass extends MyClass {
    Int c;
    Int d;

    init(Int p, Int q) {
        // Here, 'this' does not exist.
        a = 10; // Invalid, no instance yet.
        c = 10; // Invalid, no instance yet.

        super(p);
        // Here, 'this' is of the type MyClass, since the super class is created.
        a = 10; // Valid, MyClass is created.
        c = 10; // Invalid, this is of type MyClass, which does not contain 'c'.

        init {
            c = q;
            d = q + 10;
        }

        // Now we can access all members.
        a = 10; // Valid, we have an instance.
        c = 10; // Valid, 'this' is now of type MyDerivedClass.
    }
}
```

Cast Constructors
-----------------

By default, Basic Storm is restrictive with automatic typecasting. In some cases it is, however,
convenient to allow Basic Storm to automatically convert between types. To achieve this, it is
possible to declare a constructor as a *cast constructor*. This is a constructor that takes a single
parameter. Since it is marked as a *cast constructor*, Basic Storm may use it to automatically
convert from one type to another. A *cast constructor* is defined using the keyword `cast` instead
of `init`. The syntax is otherwise identical to regular constructors.


Options
-------

As with functions, it is possible to specify options to types as well. Options are specified by
adding a colon (`:`) after the type name, and specifying each option separated by commas. The
following options are provided by Basic Storm. Extensions may add more options by extending the rule
`lang:bs:SClassOption`, or by declaring a function that accepts a `core:lang:Type` as a parameter.

- `extends <name>`

  Make the type extend another type. Not possible to use together with `on`.

- `on <thread>`

  Only usable with classes. Makes the class into an actor that runs on the specified, named, thread.
  All members of the class need to execute on the specified thread. If called by other threads, the
  system automatically sends a message to the appropriate thread.

- `on ?`

  Only usable with classes. Makes the class into an actor that executes on a thread known only at
  runtime. The thread is specified as the first parameter to all constructors (a `Thread` needs to
  be specified explicitly, and it may not have a name).

- `serializable`

  Requires including `util:serialize`. Adds members required for serialization and deserialization
  of the type. Only supported for values and classes.


Two options are special: `extends` and `on`. If these are the only options, they may be specified
without the colon.
