Command-line options
=====================

By default, launching Storm `Storm` from the command line launches the REPL (i.e. an interactive
session) for Basic Storm. This behavior can be changed by using different command-line flags, or by
passing files or directories as parameters. Storm will try to run code from any files specified
directly (see below).

To start a REPL of another language, pass `-l <language>`. For example: `Storm -l bs` to start the
Basic Storm REPL. This can also be used to force starting the REPL if another option would cause
something else to happen.

To pass a string to the REPL of a language, use the flag `-c`. For example: `Storm -l bs -c 1+2` sends
the expression `1+2` to the Basic Storm REPL. This flag can of course be used without the `-l` flag.

To run a single function, use the `-f` flag. This function may not take any parameters, and not all
return types are supported. Example: `Storm -f foo.main`.

To specify a root path, use `-r`. For example: `StormMain -r root/` launches Storm with `root/` as
the root directory. The default root directory depends on your system. If you have Storm installed
in your system, it resides in `/usr/share/storm` on most systems. Otherwise, it is the `root/`
directory relative to the directory of the executable file.

It is also possible to import specific directories into the package tree. This is done with the `-i`
and `-I` flags. `-i` takes one parameter, the name of a file or directory to be imported. This file
is placed in a package with the same name as the file or directory. For example `Storm -i myfile.bs`
will import the contents of `myfile.bs` into the package `myfile`. If you wish to specify the
location of the imports, use `-I <package> <file>` instead. For example: `-I mypkg myfile.bs`
imports the file into `mypkg` instead. Both `-i` and `-I` works with both files and
directories. These flags do not alter the default behavior, so unless you specify something else, a
REPL is still launched.

Any options without a preceding flag are treated the same as `-i` (i.e. as the names of files or
directories), with one important difference: Storm will try to execute the function `main` from
these imports if they exist. So, to run the function `main` in the file `myprogram.bs`, one can type
`Storm myprogram.bs`. To launch the REPL in this context, either type `storm -i myprogram.bs` or
`storm myprogram.bs -l bs`.
