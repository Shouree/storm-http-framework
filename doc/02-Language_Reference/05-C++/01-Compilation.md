Compiling C++
=============

The annotations described in this section of the manual require two things to function correctly.
First, the file `Core/Storm.h` needs to be included to make the necessary definitions visible.
Secondly, the preprocessor needs to be executed to generate the necessary metadata.

The preprocessor can be executed in two modes: either for the compiler (typically only used when
compiling Storm itself), and for a separate library. When using mymake, this can be done
automatically.

The preprocessor (built by the `CppTypes` target in the main Storm repo) accepts a number of paths
that denote directories in which header files for Storm are located. It also accepts the following
parameters:

- `--using <namespace>`

  Specifies that the code in the headers contain `using <namespace>;` at the global level. This
  helps the preprocessor resolve types correctly.

- `--use <path>`

  Specifies paths that are scanned for dependencies, but are not exported by the library. Typically,
  one would specify `--use Core/` to allow using the standard library.

- `--template <path>`

  Specifies the location of the template file for the generated metadata. The file is located in
  `Core/Gen/CppTypes.cpp` in the Storm repository.

- `--out <path>`

  Specifies the output location of the generated metadata file. Typically located in
  `Gen/CppTypes.cpp` in the sub-project.

- `--asm <template> <output>`

  Specifies the location of the template file for the generated assembler. The file to use depends
  on the current platform. It is one of the files `Core/Gen/CppVTables.{VS_X86,VS_X64,GCC}`. The
  output specifies the location of the generated output. It is typically named
  `Gen/CppVTables.{asm,S}`.

- `--doc <output>`

  Specifies the name of the generated documentation file. Typically `root/<package>/<package>_doc`,
  in the same location as the final package.

- `--compiler`

  Specify semantics for the compiler itself.


If compiling with mymake (in particular, as a subdirectory in the Storm repository), it is enough to
specify the following in the `.mymake`-file for the project, and to specify that the configuration `stormpp` is required for the project.

```
[]
# Includes all files in the current directory.
input=*

# Specifies which Storm package the library should end up in. Applies to both
# the generated documentation, and the final dynamic library.
package=<package>

# Specifies that a namespace is being used in C++.
stormUsingNamespace=storm
```

The `stormpp` and `sharedlib` configuration options are enabled as follows in the `.myproject` file:

```
[build]
# Enable running the preprocessor.
<subproject>+=stormpp
# Compile as a shared library.
<subproject>+=sharedlib

[deps]
# Make sure that CppTypes is built.
<subproject>+=CppTypes
```

When compiling with mymake, it is also necessary to create a precompiled header file in the form of
the files `stdafx.h` and `stdafx.cpp`. The `stdafx.h` file typically includes `Core/Storm.h`, and
the `stdafx.cpp` file includes `stdafx.h` and then defines the entry point of the shared library.
As such, a typical `stdafx.cpp` file includes the following:

```
#include "stdafx.h"
#include "Shared/Main.h" // for SHARED_LIB_ENTRY_POINT

SHARED_LIB_ENTRY_POINT();
```

By default, mymake is not aware that it needs to build the project. This can be done as follows:

```
[deps]
Compiler+=<subproject>
```
