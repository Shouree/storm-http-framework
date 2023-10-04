Top Loop
=========

Storm is able to provide interactive top-loops (sometimes called REPL for Read Eval Print Loop) for
experimenting with programming languages. Whenever Storm is asked to provide an interactive
top-loop, it looks for a function `core.<ext>.repl()` that is assumed to create an instance of a
`lang.Repl` class that provides the functionality. Storm then handles input and output on
behalf of the language, and calls the appropriate members of the `Repl` class. The `Repl` class has
the following members:

* `greet`

  Called to print any greeting messages. Only called if executed interactively.

* `eval(Str)`

  Called when a line of input has been received from the user. Expected to evaluate the code and
  print the result as is appropriate for the language. If the input is incomplete, it should return
  `false`. This causes Storm to collect more input and call `eval` again with the new input appended
  to the old input.

* `exit`

  Called by Storm after each call to `eval` to determine if the top loop is ready to exit.


The existence of this interface means that it is possible to write other front-ends that provides
interactive execution. For example, it is possible to implement a GUI that submits input to the
appropriate top-loop.
