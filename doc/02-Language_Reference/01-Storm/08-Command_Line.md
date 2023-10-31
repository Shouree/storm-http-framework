Command Line Parameters
=======================

The Storm executable accepts a number of command line parameters that determine its behavior. If no
parameters are specified, the default behavior is to run the Basic Storm [top loop](md:Top_Loop)
interactively.

Parameters to Storm have the following form in general:

```
Storm <flags> <mode> [-- <arguments>]
```

Parts in angle brackets (`<>`) are explained further below. Parts in square brackets (`[]`) are
optional.

## Mode

The `<mode>` part determines how Storm should operate. If multiple modes are specified, the last one
takes precedence over the others (generally, it is only sensible to specify a single mode). The
supported modes are as follows:

- *(empty)*

  Start the Basic Storm top loop interactively.

- `-f <name>`

  Run the function with the name `<name>` and exit. This mode suppresses most output from the
  interactive mode, and is therefore suitable when launching applications.

  The name is expected to refer to a function that takes no parameters and returns either nothing or
  an integer type. If an integer type is returned, the result is returned to the operating system
  when Storm terminates.

- `<path>`

  Import the file or directory named `<path>` into the name tree as if specified using the `-i` flag
  (see below). If the imported package contains a `main` function, it will be executed as if
  specified by the `-f` flag.

  It is possible to specify multiple paths in this manner to import multiple packages. If this is
  done, Storm will attempt to find a `main` function in the order they appear on the command line,
  and execute the first one. If `-f` is specified, it explicitly overrides the automatic search for
  a `main` function.

- `-l <language>`

  Launch the top loop for `<language>`. For example, `-l bs` launches the Basic Storm top loop.

- `-c <expression>`

  Evaluate `<expression>` in the top loop currently selected. As such, it is possible to use `-c`
  alongside `-l` to select which top loop to use as follows: `Storm -l bs -c "print(1.toS)"`.

  **Note:** Depending on your shell, you might need to surround `<expression>` in quotes to avoid it
  being interpreted as multiple parameters to Storm.

- `-t <name>`

  Run the test suite in the package referred to by `<name>` and exit. The test suite is expected to
  be defined using the [test library](md:/Library_Reference/Unit_Tests).

- `-T <name>`

  Run the test suite in the package referred to by `<name>` and exit. In contrast to `-t`, this mode
  recursively traverses all packages inside `<name>` and executes any tests found there as well.

- `--version`

  Print version information and exit.

- `--server`

  Start the [language server](md:Language_Server). In this mode, Storm expects to communicate with
  the process that started it using a binary protocol. If started in this mode in a terminal, the
  output is likely to be difficult to read.


## Flags

The `<flags>` parameter affect the operation of Storm, before it launches its main task. These are
currently used to specify where the root of the name tree is located, and to import additional
packages from the file system. They are as follows:

- `-i <path>`

  Imports a package from the file system into the name tree. If the path `<path>` refers to a
  directory, the entire directory is imported as a package with the same name as the directory. If
  it refers to a file, the package will be named according to the title of the file (i.e. without
  the file's extension).

  **Note:** in contrary to specifying a path on the command line without the `-i` flag, using the
  `-i` flag does not cause Storm to search for a `main` function.

  **Note:** packages imported with the `-i` flag are always imported into the root of the name tree.
  Use `-I` to provide an explicit name.

- `-I <name> <path>`

  Import a package into the name tree with the name `<name>`. The parent package to `<name>` is
  expected to exist in the name tree already (it may be imported by an earlier `-i` or `-I` flag,
  for example). The contents of the package will be populated from the contents of `<path>`. As with
  `-i`, `<path>` may refer either to a file or a directory.

- `-r <path>`

  Specify where the root of the name tree is located in the file system. By default, Storm selects
  the default location that contains the code necessary for it to boot. To find the default
  location, start Storm in the interactive mode, and it will print the location used on your system
  in its greeting.

  It is only necessary to specify a new location for the root of the name tree if the default is not
  satisfactory, or if you wish to make large modifications to parts of the files that are
  distributed with Storm. For example, if you wish to modify the implementation of Basic Storm but
  do not wish to edit files installed in central locations of your system.

  **Note:** Certain parts of [Basic Storm](md:/Language_Reference/Basic_Storm) and
  [the Syntax Language](md:/Language_Reference/The_Syntax_Language) are implemented outside of the Storm binary.
  As such, specifying an alternate root directory without the appropriate files from the
  distribution will cause them to not function properly. As such, make sure the `lang` directory is
  present in whichever directory you specify with `-r`!


## Arguments

Any parameters specified after the delimiter `--` are made available to the Storm program through
the function `core.argv`. This function returns an array of strings that contains the parameters. It
is also possible for programs to modify this array by calling `core.argv(core.Array(core.Str))` (or
assigning to `core.argv` in Basic Storm).
