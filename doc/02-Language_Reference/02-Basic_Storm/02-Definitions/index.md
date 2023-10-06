Definitions
============

Basic Storm supports the following definitions. The definitions may occur at the top-level (after
any `use` statements) or inside type definitions unless otherwise noted. The list contains a short
example of the syntax. More information about each type of declaration can be found by following the
link in the list.


- **[Functions](md:Functions)**

  ```bs
  Int mySum(Int a, Int b) { a + b; }
  ```

- **[Types](md:Types)**

  ```bs
  class MyClass {}
  ```

- **[Enums](md:Enums)**

  ```bs
  enum MyEnum {
      a, b = 10, c
  }
  ```

- **[Named Threads](md:Named_Threads)**

  ```bs
  thread MyThread;
  ```

- **[Global Variables](md:Global_Variables)** (only at top-level)

  ```bs
  Int myGlobal on Compiler = 3;
  ```

- **[Member Variables](md:Types)** (only inside types)

  ```bs
  class MyType {
      Int myVariable;
  }
  ```

- **[Constructors](md:Types)** (only inside types)

  ```bs
  class MyType {
      Int x;
      init(Int param) {
          init { x = param; }
      }
  }
  ```

Syntax extensions may add other types of definitions by adding productions to the `SPlainFileItem`
for the top-level, or for `SClassWrapItem` for types.


Visibility Modifiers
----------------

In general, it is possible to specify the visibility for individual definitions by prepending one of
the keywords `public`, `private`, `package`, or `protected`. If none is specified, the default is
`public`. For top-level definitions, they have the following meaning:

- `public`: The definition is visible to all parts of the system.

- `package`: The definition is only visible in the current package.

- `private`: The definition is only visible in the current source file.

For definitions inside a type, they have the following meaning:

- `public`: The definition is visible to all parts of the system.

- `package`: The definition is only visible in the current package.

- `protected`: The definition is only visible to the current type, and any derived types. Note that
  in contrast to languages like C++, `protected` definitions are visible regardless of whether they
  are accessed through the `this` variable or not.

- `private`: The definition is only visible in the current type.

If multiple definitions should have the same visibility, it is possible to specify the visibility
once, before all definitions as follows:

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
