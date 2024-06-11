Libraries in C++
================

This tutorial illustrates how to create a library written in C++ that can be used from Storm. This
allows integrating third-party libraries written in C and/or C++ with Storm.

A variant of the code produced in this tutorial is available in the `TestLib` directory in the Storm
source code. The code is built by default when building Storm, and the resulting binary is placed in
the `tests.shared` package.

Setup
-----

Since we will be building code in C++, this tutorial assumes that you are able to build Storm from
source already. This process is described [here](md:../Developing_in_Storm/Compile_from_Source).

Each library needs to be a target according to mymake. As such, we need to create a subdirectory in
the root of the source code repository of Storm (i.e. as a sibling to the directories `Core`,
`Compiler`, etc.). For the purposes of this tutorial, we will use the name `Tutorial`, but you can
pick a suitable name. The name of the directory only affects the name of the produced dynamic
library, which does not matter for Storm. The library can still be placed in any package in the name
tree.

To compile the library, the following files need to be created in the new `Tutorial` directory:

- `.mymake`

  Configuration for Mymake. Describes how the library should be compiled. The following contents is
  a good starting point:

  ```
  []
  input=*
  package=tutorials/cpp
  stormUsingNamespace=storm
  ```

  The three lines of configuration have the following meaning:

  - `input=*` - Tells mymake to include all `.cpp` files in the compilation.
  - `package=tutorials/cpp` - Specifies where the compiled binary should be placed. The package is specified as
    a path relative to the `root/` directory.
  - `stormUsingNamespace=storm` - Tells the preprocessor that we have a `using namespace storm;` that is visible
    globally. Allows the preprocessor to resolve names in C++ properly. Multiple such statements can be specified.
    In such case, subsequent statements must replace `=` with `+=` to not overwrite the previous ones.

- `stdafx.h`

  Precompiled header for the library. This file is expected to be included first in all `.cpp`-files
  in the library. This allows the compiler to process the includes only once.

  The `stdafx.h` file typically has the following contents:

  ```
  #ifndef TUTORIAL_LIB_H
  #define TUTORIAL_LIB_H
  #include "Shared/Storm.h" // Includes for Storm.

  // The name "tutorial" can be anything.
  namespace tutorial {
      using namespace storm; // To make classes from Storm visible.
  }

  #endif
  ```

- `stdafx.cpp`

  This is the unit that is compiled in order to compile `stdafx.h`. It can be empty, but it is a
  good point to define the entry point for the library that is used for Storm:

  ```
  #include "stdafx.h"
  #include "Shared/Main.h"

  // Defines what we need for the library to work in Storm.
  SHARED_LIB_ENTRY_POINT();
  ```

- `Gen/.gitignore`

  The directory `Gen` will be created by the preprocessor to include some metadata. These are
  typically not relevant to include in a version control system. For that reason, it is useful to
  create a `.gitignore` file with the following contents:

  ```
  *
  !.gitignore
  ```

Finally, we need to modify the `.myproject` file in the root of the Storm source code to instruct it
to build our new library properly.

First, under the `[build]` section, add the following lines to instruct that the library needs the
Storm preprocessor, and that it should be made into a shared library:

```
[build]
#...
Tutorial+=stormpp
Tutorial+=sharedlib
```

We also need to tell mymake that our new library depends on the preprocessor by adding the following
to the `[deps]` section:

```
[deps]
#...
Tutorial+=CppTypes
```

Finally, in the `[deps,!dist]` section, add our library as a dependency of the `Compiler`
subproject. This ensures that mymake builds it automatically when we build the compiler. Without it,
we would manually have to build it using `mymake Tutorial` each time we have changed it:

```
[deps,!dist]
#...
Compiler+=Tutorial
```

With those changes made, we should be able to run `mymake Main` to build the new library. If
everything works correctly, the directory `root/tutorials/cpp` should be created, and contain the
file `DebugTutorial.so` or `DebugTutorial.dll` depending on your operating system. There should
also be a file named `Tutorial_doc` that contains the documentation from the library.


Defining a Function
-------------------

Now that we have a skeleton set up, we can create new pairs of files as usual. For example, we can
create a file `Functions.h` that contains a few conversion functions as follows:

```
#pragma once

namespace tutorial {

    Int STORM_FN bestNumber();
    Str *STORM_FN greeting(EnginePtr e);

}
```

As we can see, the functions are declared using `STORM_FN`. This tells the preprocessor that the
function should be accessible from Storm. This means that the function needs to accept types
accessible to Storm as parameters, as well as return a type accessible to storm.

The function `greeting` accepts a parameter of the type `EnginePtr`. We need this parameter since we
need access to an `Engine` object in order to be able to create objects on the heap. This type is
special. Whenever it appears as the first parameter to a non-member function, the system generates
code to transparently pass a suitable `Engine` through the `EnginePtr` parameter. This parameter
will therefore not be visible in Storm as we shall see.

We can implement these functions in the file `Functions.cpp` as follows:

```
#include "stdafx.h"
#include "Functions.h"

namespace tutorial {

    Int bestNumber() {
        return 42;
    }

    Str *greeting(EnginePtr e) {
        return new (e.v) Str(S("Hello!"));
    }

}
```

These functions do not need to be marked `STORM_FN` since the preprocessor only examines header
files. Furthermore, we can see that the `greeting` function needs to pass an instance of an `Engine`
as a parameter to the `new` operator when creating a `Str`. For the string literal, we also need to
use the macro `S(...)` in order to specify the proper character encoding for the string literal.

Now, we can build the code and run it by typing `mymake Main`. This opens the Storm top loop. We can
then run the functions by typing: `tutorials:cpp:bestNumber` and `tutorials:cpp:greeting`. These
should produce `42` and `Hello` respectively.

Printing in C++
---------------

The Storm headers include the macro `PLN` that can be used to print things to standard
output. Contents can be constructed using the `<<` operator as normal. For example:

```
PLN(L"Hello " << L"world!");
```

Note that string literals inside `PLN` statements need to be preceeded by `L` rather than `S(...)`.


Defining Types
--------------

Types are defined as usual. However, they need to be exported using `STORM_VALUE` or `STORM_CLASS`
depending on the type. For example, we can define the following types in `Types.h`:

```
#pragma once

namespace tutorial {

    class ValueType {
        STORM_VALUE;
    public:
        STORM_CTOR ValueType();

        Int val;

        void STORM_FN toS(StrBuf *to) const;
    };

    class ClassType : public Object {
        STORM_CLASS;
    public:
        STORM_CTOR ClassType();

        Int val;

    protected:
        void STORM_FN toS(StrBuf *to) const;
    };

    class ActorType : public ObjectOn<Compiler> {
        STORM_CLASS;
    public:
        STORM_CTOR ActorType();

        Int val;

    protected:
        void STORM_FN toS(StrBuf *to) const;
    };

}
```

We can see from above that the classes are defined mostly as usual in C++. The difference is that
value types need to contain `STORM_VALUE` as the first thing in the class body. Class and actor
types use `STORM_CLASS` instead. Class types need to inherit from `storm::Object`, and actor types
need to inherit from `storm::TObject` (for specifying the thread in the constructor), or from
`ObjectOn<T>` that specifies the thread statically. The system respects inheritance, so it is also
possible to inherit from other class- or value-types instead of inheriting directly from `Object` or
`TObject`.

The example above also shows that constructors need to be exposed to Storm using `STORM_CTOR`. The
exception to this rule is the copy constructor, that is exposed automatically for value types. We
can also see that it is possible to simply override `toS(StrBuf *)` to create a string
representation that works in both Storm and C++. One limitation exists: the output of value types
from C++ requires using the `StrBuf` class. It will not work with standard C++ IO-streams.

Finally, we need to implement the functions in the `Types.cpp` file as follows:

```
#include "stdafx.h"
#include "Types.h"

namespace tutorial {

    ValueType::ValueType() : val(1) {}

    void ValueType::(StrBuf *to) const {
        *to << S("Value type: ") << val;
        return to;
    }

    ClassType::ClassType() : val(2) {}

    void ClassType::toS(StrBuf *to) const {
        *to << S("Class type: ") << val;
    }

    ActorType::ActorType() : val(3) {}

    void ActorType::toS(StrBuf *to) const {
        *to << S("Actor type: ") << val;
    }

}
```

After this, we can compile and run the program using `mymake Main` as before. This time, we can test
our implementation by creating an instance of the three objects in turn: `tutorials:cpp:ValueType`,
`tutorials:cpp:ClassType`, and `tutorials:cpp:ActorType`. These should print: `Value type: 1`,
`Class type: 2`, and `Actor type: 3` respectively.

Limitations
-----------

There are a few limitations in the preprocessor regarding what types in Storm can be used. Most
notably, templates can not be used, except for the container types in the standard library
(e.g. `Array`, `Map`, ...). Furthermore, the types contained in these types can not be template
types themselves. That is, `Array<Int>` is supported, but `Array<Array<Int>>` is not. The second one
is, however, possible to achieve by creating a value type `Row` that contains an `Array<Int>` as a
value. Then the nested array can be expressed as `Array<Row>`.
