Running External Code
=====================

Programs that are larger than a few lines of code are not very convenient to run in the top-loop. It
is better to save them in files in the file system and instruct Storm to load the code from there
instead. This process is called *importing* in Storm.

As with the previous pages in this section, this page will contain examples of command lines. In
these examples, the command `storm` is assumed to start Storm. However, this may not be the case on
your system. Refer to [this page](md:In_the_Terminal) for an explanation on how to start Storm on your system.


Importing Code
--------------

There are two command-line flags for importing code, `-i` and `-I`. The `-i` flag is simpler and
often enough, so we will start by illustrating that flag.

The `-i` flag takes a single parameter on the command line: the name of a file or directory that
should be imported. Storm then creates a new package in the root of the name tree with the same name
of the file or directory, and imports the contents there.

To illustrate the process, assume that we have the following file saved as `demo.bs`:

```bs
void main() {
    print("Hello from Storm!");
}
```

To import the file into the name tree, we can run Storm as follows:

```
storm -i demo.bs
```

If done correctly, this command will launch the top-loop as usual, without any additional messages.
If you have mistyped the file name, or placed the file in a different directory than the current
directory of your terminal, then a message (`WARNING: The path ... could not be found.`) will be
displayed before the top-loop is started.

The package will be imported as the package `demo` since the name of the file was `demo.bs`. The
file `demo.bs` does not have to be located in the current working directory. The parameter to `-i`
can be an arbitrary path to locate the file (for example: `~/storm/demo.bs`, or `tests/demo.bs`).


Once in the top-loop we can verify that the file was imported by typing `help demo` for example. If
done correctly, the output will end with the following:

```
Members:
 main -> void [public]
```

We can then run the function by calling it, for example by typing: `demo:main()`


As mentioned above, the `-i` flag also works for directories. To illustrate this, let us assume that
the program has grown larger than what is convenient to store in a single file. We now have two
files in a directory called `demo`. The first one, `main.bs` contains the following code:

```bs
void main() {
    greeting("Hello");
    greeting("Welcome");    
}
```

The second file, `greeting.bs`, contains the following code:

```bs
void greeting(Str start) {
    print("-" * start.count);
    print(start);
    print("-" * start.count);
}
```

Note that the names of the files are arbitrary, they do not need to correspond to the functions in
the files as far as Storm is concerned.

As before, we can use the `-i` flag to import the code. However, since we wish to import both files
into the same package, we simply specify the directory itself rather than the individual files:

```
storm -i demo
```

As before, this imports the contents of the directory `demo` into the name tree as the package
`demo`. If the directory contains any subdirectories, they are imported as sub-packages as well. As
with files, `demo` can be replaced by a more complex path if necessary.

To verify the import, we can once again type `help demo`. The output should end with:

```
Members:
 greeting(core.Str) -> void [public]
 main -> void [public]
```

And, we can once again run the program by calling `main`: `demo:main()`. Of course we can also call
`greeting` directly if we wish: `demo:greeting("Test")`.

The `-i` flag can be repeated multiple times to import multiple files or directories.

### Using an Alternative Name

So far, we have used the `-i` flag to import things into the name tree. This is enough in the vast
majority of cases. However, it is sometimes necessary to be able to specify the location of the
imported contents in the name tree. This can be done with the `-I` flag. It takes two parameters.
The first one specifies the name in the name tree of the imported code. The second one is a path to
a file or directory that should be imported.

For example, to import the `demo` directory into the package `examples.greeting` we can write:

```
storm -I examples.greeting demo
```

This imports the contents of the directory `demo` as the package `examples.greeting`. This means
that we need to call the functions as follows: `examples:greeting:main()`.


Running Code
------------

So far, we have only used command-line parameters to import code. We still had to run the code from
the top-loop. This quickly gets tiresome, especially when developing programs. For this reason,
Storm provides a number of ways to also execute the imported code directly from the command line.

The first method is the `-f` flag. It takes a single parameter that is expected to be the name of a
function in the name tree. Since it is not possible to specify parameters to the function on the
command line, the function may not take any parameters.

The function `main` from the examples above requires no parameters, and can be called directly from
the command line. We must still import the package to be able to call it. This can be done by
combining the `-i` and `-f` flags as follows:

```
storm -i demo -f demo.main
```

Note that the order is not important. The `-f` flag is executed last regardless of the order.

If done correctly, this makes Storm import the code in the `demo` directory into the `demo` package
in the name tree, and then execute the function `demo.main` (written as `demo:main` in Basic Storm).
After `main` returns, Storm terminates. In this mode, Storm does not print any messages itself.

Since it is common to import code and then immediately run it, Storm provides a shorthand for the
flags above. If the name of a file or directory is specified as a command line argument without a
preceeding flag, then Storm imports it (as the `-i` flag would do), and then searches the imported
package for a `main` function. If a `main` function exists, it will call the function as if the `-f`
flag was specified. Otherwise, it will launch the top-loop. This means that we can run the program
as follows:

```
storm demo
```

If multiple imports are specified in this manner, Storm searches them in the order they were
specified, and runs the first `main` function that was found.


The `-f` flag can be used to run any function inside of Storm, it is not limited to functions in
imported packages. For example, Progvis can be started as follows:

```
storm -f progvis.main
```

If the function that was launched automatically returns an integer type (e.g. `Int` or `Nat`), then
the return value from the function is used as the exit code from the entire Storm process. If the
function returns `void`, then Storm exits with a status of 0 unless the function terminated by
throwing an exception that was not caught.
