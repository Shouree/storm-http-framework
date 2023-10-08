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

- **[Enumerations](md:Enumerations)**

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

- **[Generators](md:Generators)**

  ```bs
  use lang:bs:macro;

  generatedType : generate(params) {
      if (params.count != 1)
          return null;
      return Type("generatedType", params, TypeFlags:typeClass);
  }
  ```

Syntax extensions may add other types of definitions by adding productions to the `SPlainFileItem`
for the top-level, or for `SClassWrapItem` for types.


Visibility Modifiers
--------------------

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


*Note:* the `on <thread>` decorator for functions and classes can be used similarly to how
visibility modifiers can be used. That is, it is possible to specify `on Compiler:` once in the
source file, and have all functions and types be affected automatically.
