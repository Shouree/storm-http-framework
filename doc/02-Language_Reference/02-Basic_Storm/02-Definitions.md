Definitions
============

Basic Storm supports the following definitions. The definitions may occur at the top-level (after
any `use` statements) or inside type definitions unless otherwise noted. Parts that are written in
angle brackets (`<>`) are placeholders. Any parts in square brackets (`[]`) are optional.

- **Functions**

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

- **Types**

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

- **Enums**

  Basic Storm provides special syntax for enumeration types. An enumeration is a value type that
  contains an integer (`core:Nat`) that is expected to contain one out of a number of pre-defined
  values. An enum is defined as follows.

  ```
  enum <name> [: bitmask] {
       <element> [= <value>],
       ...
  }
  ```

  This defines an enumeration type named `<name>`. If the `: bitmask` part is present, the
  enumeration is a bitmask and may contain combinations of the values, rather than just a single
  value. The body of the enumeration may contain a number of elements. Each row contains the name of
  the element (`<element>`), and optionally the value of the element (`<value>`). For example, an
  enumeration may look like this:

  ```bs
  enum MyEnum {
      a = 0,
      b,
      c = 5
  }
  ```

  The values in the enum will then be accessible as `MyEnum:a`, `MyEnum:b`, etc. Similarly, a
  bitmask is defined as follows:

  ```bs
  enum MyBitmask : bitmask {
      a = 0x1,
      b = 0x2,
      c
  }
  ```

  The difference with a bitmask is that each value in the enum will be assigned a bit-pattern of
  `0001`, `0010`, `0100` etc. to make it possible to represent the presence or absence of each
  element. This may, of course, be overridden by the programmer if desired. Furthermore, a bitmask
  defines operators to manipulate the bitmask, such as `+` for adding bits, `-` for removing bits,
  and `&` (or `has`) for checking for overlap between two bitmasks.

- **Named threads** (only at top-level)

  A named thread is defined as follows:

  ```
  thread <name>;
  ```

  Where `<name>` is the name of the thread. The thread is not actually started until it is used by
  some part of the system.

- **Global variables** (only at top-level)

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



Syntax extensions may add other types of definitions by adding productions to the `SPlainFileItem`
for the top-level, or for `SClassWrapItem` for types.


Access Modifiers
----------------

In general, it is possible to specify access modifiers for individual definitions by prepending one
of the keywords `public`, `private`, `package`, or `protected`. If none is specified, the default is
`public`. Definitions marked with `public` are always visible to the entire system, and
definitions marked with `package` are only visible to the same package. The meaning of `private`
and `protected` differs between definitions at the top-level and definitions inside a type.

At the top-level, definitions marked `private` are visible to the same source file as the
definition. The keyword `protected` is not useable at the top-level.

For definitions inside a type, `private` means that the definition is only visible to other
members of the same type (or subtypes). Furthermore, `protected` means that the definition is also
visible to members of any derived types. In contrast to languages like C++, `protected` members are
always visible in derived types. It is not necessary to operate on the current object (i.e.
`this.x`) to access them.

If multiple definitions should have the same access modifier, it is possible to specify the access
modifiers once, before all definitions as follows:

```bs
private:

void f1() {}

void f2() {}

public:

void f3() {}
```

In this case, both `f1` and `f2` will be `private`, while `f3` is `public`. This approach works both
inside types, and at the top level. It is also possible to temporarily override the new default by
specifying access before an individual definition:

```bs
private:

package void f1() {}

void f2() {}
```

Above, `f1` will be `package` while `f2` will be `private`.


Other Modifiers
----------------

There are a number of other modifiers that can be applied to function- and type definitions. In
Basic Storm, these appear after the name of the type, and the name of the parameter list of a
function. The list of modifiers itself starts with a colon and contains a comma-separated list of
modifiers. For functions, this looks as follows:

```bs
void f1() : on Compiler, inline {}
```

For types, it looks as follows:

```bs
class MyClass : extends MyBase {}
```

The following other modifiers are available for **functions**:

- `pure`

  Marks the function as *pure*. This means that the system may assume that the function has no
  observable side-effects, and that the output from the function only depends on the inputs to the
  function.

  Currently, this marker is only used to determine when a copy-constructor is trivial with respect
  to the calling convention. There are, however, plans to utilize the `pure` modifier to perform
  constant folding in the future, which means that calls to `pure` functions may be elided, and the
  function body might be executed entirely at compile time rather than at run time.

- `inline`

  Marks the function as *inline*, which in Basic Storm means that it is always inlined. As such,
  this modifier is only suitable to use for short and simple functions.

- `static` (only for member functions)

  Makes the function `static`, meaning that it does not have an implicit `this` parameter, and it
  may thus not access members of the type without having another instance of the object.

- `abstract` (only for non-static member functions of classes or actors)

  Makes the function `abstract`. This means that the function is allowed to not have an
  implementation in the class. It is not possible to instantiate a class or actor that contains
  abstract member functions. A derived class that is required to override all abstract functions
  before it is possible to instantiate it.

- `override` (only for non-static member functions)

  Asserts that the function is expected to override a function in the parent class. If this is not
  the case, then an error is raised.

- `final` (only for non-static member functions)

  Enforces that any derived classes may not override the function.



The following other modifiers are available for **types**:

- `extends <name>`

  The defined type should inherit from the type `<name>`. It is possible to inherit from values,
  classes, and actors. Not usable together with `on`.

- `on <thread>`

  The defined class should be an actor. The actor is statically known to only execute on the thread
  with the name `<thread>`. Not usable together with `extends`.

- `on ?`

  The defined class should be an actor. The actor can only run on a thread that is determined at
  runtime by passing a `Thread` object as the first parameter to the constructor. Not usable
  together with `extends`.

- `serializable` (implemented in the package `util:serialize`)

  Defines the class as [serializable](md:/Library_Reference/Serialization). This modifier is
  technically a part of a library, but is common enough to mention here. Serialization is currently
  not supported for actor types, and thus only works for values and classes.


There are two special cases for types, where the `:` may be omitted: when only using `extends` or
`on`. They may be typed as follows:

```bs
class MyClass extends MyBase {}
class MyActor on Compiler {}
```

As with access modifiers, the `on` modifier can be set globally as follows:

```
on Compiler:

void f1() {}
void f2() {}
```
