A New Language in Storm
=======================

This part of the tutorial describes how to create a new language in Storm. In a way, this tutorial
can be seen as connecting the pieces used in all the previous tutorials, in order to create
something larger. It is, however, worth to note that this tutorial does *not* aim to describe all
concepts behind creating a language in detail. The goal is rather to illustrate how to implement
common patterns in Storm. That being said, if you have followed the previous tutorials, you are in a
good position to understand what will be done in this tutorial. After all, in my opinion, the hard
part of writing compilers is to manage the complexity of the problem through good abstractions.
Given good enough abstractions, the problems that a compiler need to solve are not extremely
difficult (but still not always trivial). Since this tutorial *will* suggest abstractions and use
the ones from Storm, it should not be extremely difficult to follow, even if you have not previously
studied compiler construction.

Goal
----

The end goal of this tutorial is to create a new and simple language that we call `demo`. It will be
saved in file with the extension `.demo` that Storm will understand. The language itself allows
defining functions that are callable from other languages in Storm. Each function simply evaluates
an arithmetic expression and returns the result of said expression to the caller. For simplicity,
the language only works with integers. It is, however, not extremely difficult to extend this into
other types as well. It just requires a bit more code that would make the underlying ideas less
clear.

To illustrate the language, consider the following example that defines three functions `f`, `g`,
and `h`. `f` and `g` take two parameters, while `h` only takes one:

```
f(a, b) = a + b + 1
g(a, b) = a / b
h(a) = bsfn(f(a, a), 5)
```

The first function computes `a + b + 1` and returns the result, the second evaluates `a / b` and
returns the result. The third function first calls the function `f` with the parameters `a` and `a`.
Then uses the result as a parameter to the function `bsfn` that we will implement in Basic Storm,
and returns the result from that function. Note that we do not need any type definitions, since the
language only supports integers.

As we can see from the example, the language is fairly simple. Even so, it is able to utilize the
strengths of the Storm platform: it is able to seamlessly interact with other languages in the
system. As we shall see, this is something we get almost for free from the system, and not something
we need to spend much effort to achieve.

Similarly, with the help of the language server, we get syntax highlighting almost for free as well.


Setup
-----

This tutorial requires a bit more setup compared to the previous tutorials. We first need to create
a directory, for example `language` that we use to store our project in. Inside the directory, we
need two subdirectories. We will call one of them `demo`. We will load this directory as the package
`lang.demo`, and it will contain the implementation of our language. If you have installed Storm in
your home directory, you can simply create this directory as a subdirectory of `root/lang` directly
instead. Apart from the `demo` directory, we also need a `test` directory. This is where we will
store the test program in the demo language, alongside some code in Basic Storm to call the
functions we define in our new language.

During this tutorial, we will create the files `demo.bs`, `ast.bs`, and `syntax.bnf` in the `demo`
directory. Similarly, we will create the files `test.bs` and `demo.demo` in the `test` directory.
You can create these files now if you wish, or you can create them as needed throughout the
tutorial. In summary, the directory structure should look something like this:

```
language
|- demo
|  |- ast.bs
|  |- demo.bs
|  |- syntax.bnf
|- test
   |- demo.demo
   |- test.bs
```

When you have created the directories (or some of the files), you can then run the code using the
following command. This assumes that your terminal is in the `language` directory in the hierarchy
above.

```
storm -I lang.demo ./demo ./test
```

This command asks Storm to import the directory `./demo` as `lang.demo`, and then to import the
directory `test` as `test` and then run the `main` function inside `test`. Since we have not added
any code yet, this will currently just start the top loop for Basic Storm.

Note: As mentioned earlier, if you have installed Storm in your home directory, you can place the
entire `demo` directory inside `root/lang` instead. This makes Storm load the package `demo` by
default, which means that you can omit the `-I` flag, and run Storm by simply typing `storm ./test`
instead.

As in the other tutorials, the code produced by this tutorial is available in the directory
`root/tutorials/language`. Note, however, that the code there contains some additional logic to
compensate for the fact that the `demo` directory is not loaded as `core.lang`. This makes it
possible to run the code by typing `tutorials:language:main` in the Basic Storm top-loop.


Teaching Storm to Handle `.demo`-files
--------------------------------------

The first step of creating a language is to teach Storm how to handle the file type that we wish to
use for our language. If we create a file with the extension `.demo` in the `test` directory, Storm
will actually give us a hint of how to do this. We start by creating the file `demo.demo` in the
`test` directory. We add the following content to the file to let us test our implementation later
on:

```
f() = 1
```

We also create a simple `main` function in the `test.bs` file in the `test` directory so that we
don't have to exit the top-loop every time we run Storm. For now, we simply try to call the function
`f` in our file `demo.demo` and print the result:

```bs
void main() {
    Int result = f();
    print("Result: ${result}");
}
```


If we run the program at this point (see the command-line above), we get the following messages:

```
WARNING storm::Array<storm::PkgReader*>* storm::Package::createReaders(
storm::Map<storm::SimpleName*, storm::PkgFiles*>*):
No reader for [./../language/test/demo.demo]
(should be lang.demo.reader(core.Array(core.io.Url), core.lang.Package))

@/home/storm/language/test/test.bs(28-29): Syntax error: Can not find "f" with parameters ().
```

The first message is a warning that tells us that Storm does not know what to do with files of the
type `.demo`, since there is no `reader` for the type. It then proceeds to tell us that it attempted
to find a function named `lang.demo.reader(Array<core.io.Url>, core.lang.Package)` to handle the
file type. The second error simply tells us that Basic Storm was unable to find the function `f`,
which is not surprising since we have not yet implemented the `demo` language to define the
function.

The first message is, however, interesting. It tells us that if we define a function called
`lang.demo.reader(...)`, we get the opportunity to handle the new file type! Let's try implementing
it by adding the following code to the `demo.bs` file in the `demo` directory (that we load into the package `lang.demo` as indicated by the warning):

```bs
use core:io;
use core:lang;

lang:PkgReader reader(Url[] files, Package pkg) on Compiler {
    print("We should load ${files}");
    lang:PkgReader(files, pkg);
}
```

If we run the program again, we will now see that the warning has disappeared, and is now replaced
by the message we printed in the `reader` function:

```
We should load [/home/storm/language/test/demo.demo]
```

What happened is that Storm found a file with the extension `.demo` when it attempted to load the
code in `test` directory. Since Storm does not know how to deal with specific file types, it looked
for the function `lang.demo.reader(...)` and found the function we just defined. Storm then called
our function to create a `reader` for the files that it found, which is when the `print` function
was called.

In fact, Storm will call our `reader` function exactly once for every package that contains at least
one file with the extension `.demo`. If a package contains more than one file with the `.demo`
extension, they will all be passed in the array to the `reader` function. This makes it possible to
the language to assign special meaning to certain files if the language choses to do so. For
example, we might wish to make the file `package.demo` specify packages that are imported to all
other files, or contain special constructs that require it to be processed before the other files.
Since Storm does not make assumptions about how the language works, it simply gives the language a
list of all files of the specified file type that was found, and lets the language implement the
semantics.


Implementing a Reader
---------------------

As we saw before, the `reader` function is expected to create a `PkgReader` class and return it to
Storm (we will simply refer to it as a Reader). Storm will then call the member functions in the
returned reader class to coordinate the loading process between different languages in order to
resolve dependencies between languages. As such, we are *not* supposed to read and parse the files
passed in the `files` array already in the `reader` function.

Since different files in our `demo` language can be parsed independently of each other, we can use
the `FilePkgReader` class provided by Storm. The `FilePkgReader` is an implementation of a
`PkgReader` that simply contains a list of `FileReader` classes, one for each file, and forwards
calls from the `FilePkgReader` to each individual `FileReader`. It also assumes that each file
contains text, and helps us with loading the contents of each file as well.

It is, of course, possible to skip this indirection, if desired. This does, however, mean that we
need to load the contents of the files ourselves, and iterate through them when loading the contents
of each file.

The first step is to create a class that inherits from `lang:FileReader`:

```bs
class DemoFileReader extends lang:FileReader {
    init(lang:FileInfo info) {
        init(info) {}
    }
}
```

As we can see, the class receives a `FileInfo` object. This object contains, among other things, the
contents of the file as a string, the path of the file, and the package we should store the contents
of the file into. We simply pass it along to the `FileReader` constructor, which stores it as the
member variable `info`, that is accessible to the derived class.

We can then instruct the system to create our class by replacing the contents of the `reader`
function with the following:

```bs
lang:PkgReader reader(Url[] files, Package pkg) on Compiler {
    lang:FilePkgReader(files, pkg, (x) => DemoFileReader(x));
}
```

This line creates a `FilePkgReader`, and instructs it to create a `DemoFileReader` for each file in
the `files` array by passing it a lambda function that creates the appropriate type. The parameter
`x` is the `FileInfo` that the `FilePkgReader` creates for each of the files.

As mentioned before, Storm uses the created readers to coordinate the loading process between
different language. It does this by calling the functions in the reader in the following order:

- `readSyntaxRules` - Asks the reader to load any syntax rules in the file/package.

- `readSyntaxProductions` - Asks the reader to load any syntax productions in the file/package.

- `readTypes`- Asks the reader to load any types in the file/package.

- `resolveTypes` - Asks the reader to resolve any dependencies between types in the file/package. A
  typical example is to lookup the specified super-type of all types. Doing this earlier might fail,
  since the file that contains the referred type might not yet be loaded.

- `readFunctions` - Asks the reader to load any functions in the file/package. This is done after
  loading types, since it is necessary to look up parameter types to create function entities.

- `resolveFunctions` - Asks the reader to resolve any references between functions.

Since the `demo` language only defines functions, we only have to worry about `readFunctions`. As
such, we can override the function and add a call to the `print` function to see that our reader is
working as intended:

```bsclass
void readFunctions() : override {
    print("Should read functions from ${info.url}");
}
```

If done correctly, we should get the following output:

```
Should read functions from /home/filip/Projects/storm/language/test/demo.demo
@/home/storm/language/test/test.bs(28-29): Syntax error: Can not find "f" with parameters ().
```

Parsing the Language
--------------------
