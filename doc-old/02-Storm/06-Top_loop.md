Top loop
=========

Top loops are defined for each language in the class `lang.<ext>.Repl`. This class is assumed to
inherit from `core.lang.LangRepl`. When launching the compiler from the command line, it tries to
start the top loop of Basic Storm. This can be changed by passing the name of another language as a
parameter to the compiler.
