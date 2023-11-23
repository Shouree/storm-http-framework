The Top Loop
============

When `Storm` is launched without any parameters, it launches the top loop. In this mode, the system
provides an interactive prompt, similar to a shell. This prompt allows evaluating expressions in
Basic Storm, both to test the language, and to interact with the system as a whole.

Note that the command `Storm` might need to be modified on your system. See [this
page](md:In_the_Terminal) for more details.


When you launch Storm interactively in this manner, the system prints a greeting message similar to
the one below:

```
Welcome to the Storm compiler!
Root directory: .../root
Compiler boot in 64.58 ms
Type 'licenses' to show licenses for all currently used modules.
bs>
```

The last line (`bs>`) is the prompt. The characters before the angle bracket (`>`) specifies the
language that is currently used in the top loop. The letters `bs` corresponds to the file extension
used by [Basic Storm](md:/Language_Reference/Basic_Storm). This is the default language since we did
not specify anything else.

We can now type expressions in Basic Storm to evaluate them. For example, if we type the expression:
`"Hello" * 4`, we will get the following output:

```
bs> "Hello" * 4
=> HelloHelloHelloHello
```

As we can see, Storm evaluated the expression and printed the result on the next line, after the
arrow (`=>`). We can also see that the arrow is not a part of the result. Finally, Storm prints a
new prompt to indicate that it is ready for the next command. If we type `print("Hello")` we get the
following output:

```
bs> print("Hello")
Hello
=> <void>
```

The first line (`Hello`) is the output from the `print` statement. The next line starts with an
arrow (`=>`). Similarly to the previous example, this indicates the result of the expression we
entered. Since the `print` does not return anything, the system prints `<void>` to inform us of this
fact.

The prompt in the top loop supports all expressions that may appear inside a function in Basic
Storm. For example, it is possible to call arbitrary functions in Storm, load extensions to Basic
Storm, and otherwise interact with the system. For example, Progvis can be launched by typing:
`progvis:main()`.

One limitation is that it is currently not possible to define functions and types in the top loop.
It does, however, provide some additional functionality as follows:

- `exit`

  Exit the top loop.

- `use <name>`

  Include an additional package. For example, including `layout` means that all names in the
  `layout` package are visible without the `layout:` prefix. It also makes all syntax extensions
  from the `layout` package available in the top loop.

  The packages `lang:bs` and `lang:bs:macro` are included by default for convenience.

- `<var> = <expr>`

  Create a new variable in the top loop. The variable is available for subsequent commands. For
  example, the variable `x` can be defined as follows:

  ```
  bs> x = 5 + 5
  x = 10
  ```

  As can be seen above, the system responds differently to indicate that a new variable has been
  created (i.e. `x = 10` instead of `=> 10`).

- `del <var>`

  Delete a previously created variable.

- `variables`

  List all created variables and their values.

- `help <name>`

  Display the documentation for the named entity `<name>`. If `<name>` is omitted, a short summary
  of the commands in the top-loop are displayed.

- `licenses`

  Print a summary of the licenses for all modules that are currently used by the system. The command
  `fullLicenses` can be used to view the full license texts. Alternatively, one can type
  `named{<license>}` to view only a single license.

As a sidenote, the commands `exit`, `variables`, and `licenses` are implemented as regular functions
that are visible in the top loop. That is why `=> <void>` is printed after them.
