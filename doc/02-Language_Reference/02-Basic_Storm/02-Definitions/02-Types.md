Types
=====

A type is defined using the keyword `value` for values or `class` for classes and actors.
Defining a value is done as below:

```
value <name> {
    <members>
}
```

The code above defines the value named `<name>`. `<members>` is a list of definitions of the
members of the value. These can be any definitions in this list, with the exception of those that
are explicitly excluded.

A class is defined as follows:
  
```
class <name> {
    <members>
}
```

Similarly to the definition of a value, `<name>` indicates the name of the class, and `<members>`
is a sequence of members of the class.

Finally, an actor is defined as follows:

```
class <name> on <thread> {
    <members>
}
```

The only difference from a class definition is that the actor has the `on` modifier (see below for
details) that indicates that the actor is associated with the thread named `<thread>`. If
`<thread>` is `?`, then the actor is associated with a thread at runtime.

- **Member variables** (only inside types)

  Inside of types, it is possible to define member variables. These have the form:

  ```
  <type> <name> [= <initializer>];
  ```

  Similarly to a global variable declaration, `<type>` specifies the type of the variable, and
  `<name>` specifies the name of the variable. Since the variable is associated with an object,
  there is no need to associate the variable with a particular thread.

  If an initializer is specified, the variable is automatically initialized using the expression in
  `<initializer>` in the constructor. It is, however, possible to override any initializers by
  explicitly specifying other values in the initializer block in the constructor.

- **Constructors** (only inside types)

  Constructors are treated specially in Basic Storm. They are defined as follows:

  ```
  init(<parameters>) {}
  ```

  Similarly to function declarations, `<parameters>` is a list of comma-separated parameters to the
  constructor. If the constructor is inside an actor that requires a thread to its constructor, the
  first parameter must be of the type `Thread`.

  Furthermore, constructors need to contain an initialization block to actually create the object.
  An initialization block looks as follows:

  ```
  init[(<parameters>)] {
     <name> = <initializer>;
     <name>(<ctor-parameterss>);
     ...
  }
  ```

  The initializer block itself may specify parameters that are passed to the constructor of the
  super class (`<params>`). The contents of the block specifies which variables to initialize, and
  what values they should take. There are two forms of initializers as shown above. The first form
  initializes the variable by assigning it a copy of whatever the `<initializer>` expression
  produces. The second form instead calls the constructor that accepts the list of
  `<ctor-parameters>` specified. If a member variable does not appear in the initializer block, it
  is initialized with the initializer expression in the member variable declaration if one exists,
  or by default-constructing it.

  It is also possible to separate initialization of the superclass and initialization of the current
  class. This is done by explicitly calling the parent class' constructor by using
  `super(<parameters>)` instead of specifying them to the `init` block.

  The reason for requiring explicit initialization blocks is to give control to the programmer. In
  particular, this lets Basic Storm know the status of the initialization in the code, and avoid
  access to non-initialized variables. This is achieved by updating the `this` parameter to reflect
  the initialization state as follows:

  ```bs
  class MyBase {
      Int b;

      init(Int b) {
          // Here, 'this' does not exist since we have not yet created an instance.
          init {
              // This works as expected:
              // - The left hand side has to refer to a member variable.
              // - The member variable 'b' does not yet exist here.
              b = b;
          }
          // Here, 'this' exists and is of the type MyBase. We can
          // thus access the member variable 'b' as normal.
      }
  }

  class MyType extends MyBase {
      Int a;
      Int c;

      init(Int p) {
          // Here, 'this' does not exist since we have not yet created an instance.
          super(p);
          // Here, 'this' exists and is of the type MyBase, since we have
          // only initialized the base class at this point. We can therefore
          // access 'b' from the base class from here on.
          init {
              a = b;
              c = p;
          }
          // Here, 'this' exists and is of the type MyType, since we have
          // initialized MyType. We can therefore access 'a' and 'c' as normal.
      }
  }
  ```
