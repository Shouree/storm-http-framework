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
|
|- demo
|  |- ast.bs
|  |- demo.bs
|  |- syntax.bnf
|
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

To actually define some functions with our language, we need to parse the text in the source files.
Storm itself lets the language choose how to do this, but it is usually a good idea to use the
Syntax Language that is already available. As such, we will write grammar in the `syntax.bnf` file
in the `demo` directory that we created earlier.

### Delimiters

When writing grammar, it is usually a good idea to start by considering what constitutes whitespace
in the language. In the Syntax Language, we express this by defining the *delimiters* at the top of
the file to match our expectations. Note that it is convenient to make the comment syntax a part of
the delimiters. That means that comments may appear at any place where whitespace may appar by
default, and we do not have to think about comments in the rest of the grammar.

For our simple language we will only need the optional delimiter. Since we don't want the language
to be sensitive to whitespace, we consider any sequence of whitespace characters (space, newline,
linefeed, tab) to be a delimiter. For comments, we use the single-line C++-style comments (`//`). We
implement this as the rule `SDelimiter` with a single production as follows:

```bnf
void SDelimiter();
SDelimiter : "[ \n\r\t]*" - (SComment - SDelimiter)?;
```

The production first matches zero or more whitespace character. After that, we optionally match a
`SComment` followed by another delimiter. The recursive nature of this rule means that we will
automatically match zero or more lines of comments, optionally separated by whitespace. Note that we
use the dash (`-`) as a separator in this rule. Since we will use `SDelimiter` as the comma
operator, it would be a bad idea to use the comma as a delimiter inside itself.

The `SComment` rule can be defined as follows:

```bnf
void SComment();
SComment : "//[^\n]*\n";
```

Since the rule contains a single regular expression token it could be inlined directly into
`SDelimiter`. This structure is, however, more readable, and makes it easier to add other comment
styles in the future if desired.

Now that we have defined our delimiter syntax, we can tell the Syntax Language that we wish to use
it whenever an optional delimiter appears (i.e. `,`):

```bnf
optional delimiter = SDelimiter;
```

Since we have only defined the optional delimiter, we will not be able to use the required delimiter
(`~`) in the syntax file. This is fine in our case, since the language is simple enough to not need
it.

For languages that need a required delimiter we would define that in a similar way. The difference
is that we create a rule that always matches at least one whitespace. A first step towards creating
such a rule is to change the `*` to a `+` in the production for `SDelimiter` above. This would,
however, require at least one whitespace *after* and *between* a comments as well. This can be
solved by making the recursive step use `SDelimiter`. This is still not exactly what we might want,
since that rule would require a space before any comment. This can, however, be considered as close
enough, since comments rarely appear in places where whitespace is required, and when it is needed
it is often natural to add a space before the comment anyway.

### Parsing the Language

After defining the delimiters, we are ready to define the syntax of our language. We start
prototyping the syntax, and worry about the syntax transforms later. In this example, we start at
the top-level and define the rule for the program as a whole. We call this rule `SRoot` since it
will be the root of the parse tree. Since files in the `demo` language simply contains zero or more
function definitions, we can write it as follows:

```bnf
void SRoot();
SRoot : , (SFunction, )*;
```

The production might look a bit strange since the commas seems to be mismatched. However, recall
that we defined the optional delimiter (`,`) to match the `SDelimiter` rule. This means that we can
also write the production as:

```bnf
SRoot : SDelimiter - (SFunction - SDelimiter)*;
```

That is, the rule matches zero or more `SFunction`s, surrounded by `SDelimiter`s. The `SDelimiter`
at the start and end of the root rule are important since we want to allow arbitrary whitespace to
appear at the start and end of the source file. Not doing that would most likely confuse the users
of our language.

Next up is the rule for function definitions, `SFunction`. A function consists of an identifier that
specifies the name of the function, a list of zero or more parameters, and an expression that
defines the body of the function. We can write this as follows in the Syntax Language:

```bnf
void SFunction();
SFunction : SIdentifier, "(", SParamList, ")", "=", SExpr;
```

As you see, we refer to other rules for the function's name and the parameter list. Using the rule
`SIdentifier` for the name is convenient since it allows us to define the allowed names in one
place, so that it is easier to modify it in the future. It also helps readability. The `SParamList`
for matching the list of parameters is for readability, but also to make it easier to implement the
repetition as we shall see.

The `SIdentifier` rule can simply be implemented like this:

```bnf
void SIdentifier();
SIdentifier : "[A-Za-z]+";
```

The `SParamList` is perhaps a bit less obvious at first. The goal is to match a list of
comma-separated identifiers. This requires using multiple productions. The easiest way is perhaps to
have a special case for when the list is empty, and use the repetition syntax for the general case.
It is explained in further detail in the [grammar tutorial](md:Using_Grammar).

```bnf
void SParamList();
SParamList : ; // Zero parameters
SParamList : SIdentifier - (, ",", SIdentifier)*; // One or more parameters
```

Now, the only thing that remains is the grammar for expressions. For this, we can more or less reuse
the rules from the [grammar tutorial](md:Using_Grammar):

```bnf
void SExpr();
SExpr : SExpr, "\+", SProduct;
SExpr : SExpr, "-", SProduct;
SExpr : SProduct;

void SProduct();
SProduct : SProduct, "\*", SAtom;
SProduct : SProduct, "/", SAtom;
SProduct : SAtom;

void SAtom();
SAtom : "-?[0-9]+";
SAtom : "(", SExpr, ")";
```

To make our language a bit more interesting, we add the following productions to `SAtom` to support
variables and function calls as well:

```bnf
// Variable access:
SAtom : SIdentifier;
// Function call:
SAtom : SIdentifier, "(", SActuals, ")";

void SActuals();
SActuals : ;
SActuals : SExpr - (, ",", SExpr)*;
```

As you have probably noticed, the `SActuals` follows the same structure as `SParamList` in order to
match comma-separated parameters.

Now that we have the grammar, we just need to use it to parse the text in our reader. We can do this
by replacing the `print` call with the following code in the `readFunctions` function:

```bsclass
void readFunctions() : override {
    Parser<SRoot> parser;
    parser.parse(info.contents, info.url, info.start);
    if (parser.hasError)
        throw parser.error();
}
```

The three parameters to `parse` are as follows: the first one is the string to parse. The
`FileReader` has read the file for us and placed it in `info.contents`. The second parameter is the
path of the file. Again, we just read it from `info.url`. The third parameter is an iterator into
the string that specifies where to start. For our purposes this is not important, since the iterator
will always be to the start of the string. However, if we wish to parse the file in multiple steps
(to support extensible syntax), we need to chain file readers. For this to work properly, it must be
possible to specify that a secondary file reader should start partway through the file. Therefore it
is a good idea to pass the iterator in `info.start` to avoid future confusion.

If you have done everything correctly, you will receive the same error as before. However, we can
verify that we are actually parsing the source file by introducing an error in the `demo.demo` file.
For example, replacing the first line with `f() = a 1` produces a parse error as we would expect.

When you have verified the behavior, remember fix any errors you have introduced before continuing
the tutorial in order to avoid unnecessary troubleshooting down the line!

Extracting Information from the Parse Tree
------------------------------------------

The next step is to actually use parse tree to create functions. To do this, we need to add
transform functions to the syntax we just created. We can do this one step at a time and test it
along the way. The first step is to extract the functions in the file. To do this, we first create a
class that will represent our functions. We add it to the file `demo.bs` in the `demo` directory:

```bsclass:use=lang.bs.macro
class DemoFunction extends Function {
    Str[] paramNames;

    init(SrcPos pos, Str name, Str[] paramNames) {
        Value[] valParams;
        for (x in paramNames)
            valParams << named{Int};

        init(pos, named{Int}, name, valParams) {
            paramNames = paramNames;
        }
    }
}
```

Since we used the `named{}` syntax, we also need to add `use lang:bs:macro;` to the top of the file.

The class `DemoFunction` inherits from the `Function` class in Storm. This class is Storm's generic
representation of functions in the name tree. This means that we will be able to add the created
instances of `DemoFunction` to the name tree, and make it possible for other languages to find them
later on. This is how Basic Storm is able to find our created functions later on.

The constructor accepts three parameters. A `SrcPos` that contains the position of the function
definition, a `Str` that contains the name of the function, and an array of strings that contain the
parameter names.

In the constructor we then forward these parameters to the constructor of the parent class with some
minor modifications. As we can see, the constructor of the `Function` class accepts four parameters:
the position of the definition, the return type of the function, the name of the function, and the
types of the parameters to the function. This means that we can simply forward the `pos` and `name`
parameters. The `paramNames` need some care, however. The `Function` class only requires an array of
the *type* of all parameters, but we receive only the *names* of the parameters. Since all variables
in the `demo` language are integers, we can simply create an array with the correct number of
references to the `Int` type. We do, however, save the names of the parameters for later. Finally,
the return type is always `Int` since we always work with integers.

Now, we can turn to our grammar. The goal is to create a `DemoFunction` class from the grammar. To
determine what we need, we can start by modifying the `SFunction` rule:

```bnf
DemoFunction SFunction();
SFunction => DemoFunction(pos, name, params)
  : SIdentifier name, "(", SParamList params, ")", "=", SExpr;
```

Since we assume that the `SIdentifier` and `SParamList` rules return something useful, we also need
to modify them. The `SIdentifier` is quite easy to fix, we just need to capture the regular
expression and return that:

```bnf
Str SIdentifier();
SIdentifier => x : "[A-Za-z]+" x;
```

The `SParamList` requires a bit more thought, as we want it to return an array of the parameters. If
we would have managed to match all identifiers in the same repeat syntax, we could simply capture
that token and return it since the Syntax Language would turn it into an array for us. This is,
however, not the case. Fortunately, we can use the arrow (`->`) syntax to have the Syntax Language
call `push` on an array for us to create the array we are after:

```bnf
Array<Str> SParamList();
SParamList => Array<Str>() : ;
SParamList => Array<Str>() : SIdentifier -> push - (, ",", SIdentifier -> push)*;
```

Finally, we also need to modify the `SRoot` production to create an array of all function
definitions in the file. We can do this using a similar approach to above, or by simply capturing
and returning the `SFunction` token:

```bnf
Array<DemoFunction> SRoot();
SRoot => Array<DemoFunction>() : , (SFunction -> push, )*;
```

*or*

```bnf
Array<DemoFunction> SRoot();
SRoot => functions : , (SFunction functions, )*;
```

Now that we are done with the transform functions in the grammar, we just need to call the transform
functions and add the created `DemoFunction` instances to the name tree. We can do this by updating
the `readFunctions` function as follows:

```bsclass
void readFunctions() : override {
    Parser<SRoot> parser;
    parser.parse(info.contents, info.url, info.start);
    if (parser.hasError)
        throw parser.error();

    DemoFunction[] functions = parser.tree().transform();
    for (f in functions)
        info.pkg.add(f);
}
```

If done correctly, this will cause the previous error to disappear. However, it is replaced by a
crash. The reason for the crash is that Basic Storm found the function and attempted to call it.
However, since we did not add any code to our function, the system crashes when we try to
call the function (the function points to the address 0, which causes the crash).


Setting Up Code Generation
--------------------------

To avoid the crash, we need to add some code to our function. We do this by creating an instance to
one of the subclasses to the [stormname:core.lang.Code] and adding it to the function. In
particular, we embrace Storm's lazy-loading strategy by using the subclass
[stormname:core.lang.LazyCode] that implements this behavior for us by calling a callback function
whenever the function is called the first time. We can do this by adding the following line to the
end of the constructor in `DemoFunction`:

```bsstmt
setCode(LazyCode(&this.createCode));
```

The expression `&this.createCode` creates a pointer to the member `createCode` in the current
instance of the class. As such, we need to define the function `createCode` as well. For now, we
just throw an exception to avoid the previous crash:

```bsclass
private CodeGen createCode() {
    throw InternalError("TODO: Generate code for ${name}.");
}
```

At this point, if we run the program, we will see that the message "TODO: Generate code for f."
appears along with a stack trace. This means that our implementation works as expected! To verify
the lazy loading, we can surround the entire body of the `main` function in an `if (false)`
statement:

```bsstmt
if (false) {
    Int result = f();
    print("Result: ${result}");
}
```

Since we never *call* `f` above, the system will never try to generate code for it. Therefore we
don't see the exception in this case. Basic Storm will, however, still look up the function `f` in
order to type-check the body of the `if` statement, so the absence of the exception does not mean
that `f` was never created.

As before, remember to remove the `if (false)` you added to avoid spending time looking for problems
later on.


Generating Code
---------------

The next step is to actually generate some code for the function. This might seem like a difficult
task at first. However, given a good structure, it is possible to break the problem into small
enough pieces so that each piece is trivial to implement, and the complexity of the problem is
handled by the structure.

We will implement a simplified version of the structure used in Basic Storm for our `demo` language.
This structure is what is typically called an Abstract Syntax Tree (AST). As in Basic Storm, we base
the AST around a class we call `Expr`. The `Expr` class will be an abstract class that represents
some expression in our language. We implement the class as follows in the file `ast.bs` in the
`demo` directory:

```bs
use core:asm;
use core:lang;
use lang:bs:macro;

class Expr on Compiler {
    SrcPos pos;

    init(SrcPos pos) {
        init { pos = pos; }
    }

    void code(CodeGen to) : abstract;
}
```

As you can see, the `Expr` class is very small. It only contains a `SrcPos` that stores the location
in the source code where the expression appears. This is not necessary for code generation. However,
having a source location for all nodes in the AST makes it much easier to produce good error
messages.

The important function in the `Expr` class is the function `code`. When called, this function should
generate code to evaluate the expression that the class represents, and store it in the listing in
the `CodeGen` object passed as the parameter. One important question is, however, yet unanswered.
**Where** is the result of evaluating the node stored? As we saw in [previous
tutorials](md:New_Expressions), Basic Storm uses the `CodeResult` object to specify the location of
the result. The `code` function here does not, so where is the result stored? To make the
implementation simpler, we once again utilize the fact that we only work with integers. This means
that we can simply decide that the `code` function always stores the result of the evaluation in the
`eax` register.

It is possible to generalize this scheme a bit to support all numeric types as well as class- and
actor types. We can observe that all of these types can be stored in a single register (numeric
types fit in a register, and classes and actors are handled by pointer). This means that such a
language can simply place results in the `a` register, resized to the type being created (i.e.
registers `al`, `eax`, `rax`, or `ptrA` depending on the size). It is, however, not possible to use
this scheme to support value types, since they do not always fit in registers. This is why Basic
Storm uses `CodeResult` objects instead.

To illustrate this idea, let's start by implementing an AST node for literals. It can be implemented
as follows:

```bs
class Literal extends Expr {
    Int value;

    init(SrcPos pos, Str value) {
        init(pos) {
            value = value.toInt();
        }
    }

    void code(CodeGen to) : override {
        to.l << mov(eax, intConst(value));
    }
}
```

The interesting part of the `Literal` class above is the implementation of the `code` function.
Remember that we specified that the `code` function should evaluate the node and place the result in
the `eax` register. Evaluating a literal is trivial: literals evaluate to themselves. This means
that the only thing that remains is to store the result in the `eax` register. We do this by adding
a `mov` instruction to the listing in the `CodeGen` object.

Since Storm is lazily compiled, we can test the implementation of our literal without implementing
the semantics for the rest of the language. The lazy compilation means that Storm only attempts to
compile and type-check the code that is actually used. As such, it does not matter that most of the
semantics are missing, and would cause type errors as long as we don't use that part of the
semantics!

So, let's do the minimal amount of work needed so that we can test our implementation, and
understand how it works before attempting to implement the rest of the semantics of the language. We
can do this by starting from the `DemoFunction` class and let the error messages by Storm direct us
to what needs to be done next. Let's start by modifying the constructor to accept and store a
`SExpr` parameter for the body:

```bsclass:use=lang.bs.macro
class DemoFunction extends Function {
    Str[] paramNames;
    SExpr body;

    init(SrcPos pos, Str name, Str[] paramNames, SExpr body) {
        Value[] valParams;
        for (x in paramNames)
            valParams << named{Int};

        init(pos, named{Int}, name, valParams) {
            paramNames = paramNames;
            body = body;
        }

		setCode(LazyCode(&this.createCode));
    }

    // ...
}
```

Note that we store the body as `SExpr` rather than `Expr`. The type `SExpr` refers to the type
created by the Syntax Language to create a parse tree starting in the rule `SExpr`. This means that
we wish to store the representation of the parse tree *before* it has been transformed into nodes of
our choice. The reason for this is that we wish to do as little work as possible until we are
actually asked to generate the machine code for the function. Currently, it would not matter much.
However, when we add function calls we will try to look up names during the syntax transforms. If we
do that too early, the lookups might fail because the function we try to find has not been added
yet.

The next step is to implement the `createCode` function to generate code. Since functions in the
`demo` language are supposed to evaluate and return the associated expression, we can simply call
the `code` function of the root node in the AST to do the heavy lifting for us. We get the root node
by calling `transform` on the `body` variable we just saved. What remains is to create a `CodeGen`
object, emit a function prolog, and to make sure that the function returns the right result. The
last step requires us to remember that we decided that `code` functions store the result of
evaluating the expression in the `eax` register:

```bsclass
private CodeGen createCode() {
    // Create a CodeGen instance. All parameters are members
    // in the Function class.
    CodeGen gen(runOn, isMember, result);

    // Emit the function prolog.
    gen.l << prolog();

    // Transform the body into AST nodes, and generate code
    // for the expression.
    Expr e = body.transform();
    e.code(gen);

    // Since the result of the evaluation is stored in 'eax'
    // by our convention, we can just generate code to return
    // the value in 'eax' here.
    gen.l << fnRet(eax);

    // Return the generated code. The LazyCode will then
    // compile the generated code into machine code and
    // execute it.
    return gen;
}
```

Note that we also need to add `use core:asm;` to the top of the file. Otherwise names such as `eax`
will not be visible.

If we run the test program at this point, we will get the following error:

```
@/home/storm/language/demo/syntax.bnf(249-355): Syntax error:
Can not find "DemoFunction" with parameters (core.lang.SrcPos&, core.Str&, core.Array(core.Str)&).
```

This error is because we now need to pass the `body` to the constructor of `DemoFunction`. Since we
wish to capture the original parse tree here, and *not* transform it first, we need to add the `@`
character when capturing the body. This looks like the code below:

```bnf
DemoFunction SFunction();
SFunction => DemoFunction(pos, name, params, body)
  : SIdentifier name, "(", SParamList params, ")", "=", SExpr @body;
```

If we run the program again, we get a different error:

```
@/home/storm/language/demo/demo.bs(1039-1040): Syntax error:
No appropriate constructor for lang.demo.Expr found. Can not initialize 'e'.
Expected signature: __init(lang.demo.Expr, void)
```

This error is from the line where we assign the result of `body.transform()` to the variable `e`.
The reason is that the rule `SExpr` returns `void`, and not `Expr` as we expected. We fix this by
changing the return type of the `SExpr` rule:

```bnf
Expr SExpr();
```

This causes yet another error to appear:

```
@/home/storm/language/demo/syntax.bnf(638-654):
Syntax error: No return value specified for a production that does not return 'void'.
```

In this case, it is caused by the production `SExpr : SProduct` not returning anything. We can fix
the error by adding suitable transforms to forward the result:

```bnf
SExpr => x : SProduct x;
```

If we continue to run the program and fix errors like above, we will see that we also need to change
the result of `SProduct` and `SAtom` in the same way, and that we need to modify the productions
`SProduct : SAtom` and `SAtom : "-?[0-9]+";` as follows:

```bnf
SExpr SProduct();
// ...
SProduct => x : SAtom x;
// ...
Expr SAtom();
SAtom => Literal(pos, num) : "-?[0-9]+" num;
```

After these modifications, the program finally runs and produces the output:

```
Result: 1
```

This is exactly what we would expect, and it means that our new language can successfully compile
and run very simple functions! A great first step!

Implementing Operators
----------------------

Now that our implementation has progressed far enough to compile simple functions, let's implement
operators as well. This will show the strength of our assumptions regarding the `code` function,
since this will make it fairly straight-forward to implement the operators correctly, regardless of
the complexity of the expressions that are being evaluated.

To start, let's update our test program in the `demo.demo` file in the `test` directory as follows:

```
f() = 10 + 4
```

If we try to run the example now, we will get an error similar to before that says that the
production for `+` does not return anything, even though it should return a non-void value. We can
fix that by updating the production as follows:

```bnf
SExpr => OpAdd(pos, l, r) : SExpr l, "\+", SExpr r;
```

Of course, we must also define the class `OpAdd` as well. The first part of the class is
straight-forward, we simply store the expressions that correspond to the left- and right-hand sides
of the operator:

```bs
class OpAdd extends Expr {
    private Expr lhs;
    private Expr rhs;

    init(SrcPos pos, Expr lhs, Expr rhs) {
        init(pos) {
            lhs = lhs;
            rhs = rhs;
        }
    }
}
```

The `code` function is, however, at first a bit trickier. In order to evaluate the `+` operator, we
must first evaluate the left- and right hand sides in some order. Then we can add the results of
these expressions together.

At this time, we need to remember the convention we established: that the `code` function always
places the result in the `eax` register. This allows us to implement `+` using these steps:

1. Evaluate `rhs`. The result is stored in `eax`.
2. Store `eax` in a temporary variable, so that its value is not overwritten in the next step.
3. Evaluate `lhs`. The result is stored in `eax`.
4. Add the value in the temporary variable to `eax`. The result is now in `eax`.

We can implement this strategy as follows:

```bsclass
void code(CodeGen to) : override {
    // Create a temporary variable for step 2.
    Var tmp = to.l.createIntVar(to.block);

    // Step 1:
    rhs.code(to);
    // Step 2:
    to.l << mov(tmp, eax);
    // Step 3:
    lhs.code(to);
    // Step 4:
    to.l << add(eax, tmp);
}
```

If we run the program now, it produces the correct result: 14. It seems like our `+` operator is
working!

One question that might arise at this point is: Why do we need to create a temporary variable? Is it
not enough just store the temporary variable in another register, like `ebx` or `ecx`? It is indeed
true that this would be enough in the case where only a single `+` appears in the expression.
However, consider what would happen for the expression `1 + 2 + 3 + 4` for example. Since all `+`
operators attempt to store their temporary inside the `ebx` register, they will overwrite each
other's stored values, and produce the wrong result. You can see this effect by replacing the line
`Var tmp = ...` with `var tmp = ebx;` and printing the `gen` variable in the `createCode` function
in the `DemoFunction` class.

We can now implement the remaining operators in the same way. This will allow us to evaluate
arbitrary expressions, for example `f() = (10 + 4) / 2`. This example also highlights that we need
to add a transform function to the production for parentheses as well. All in all, the expression
grammar should look like below after these modifications:

```bnf
Expr SExpr();
SExpr => OpAdd(pos, l, r) : SExpr l, "\+", SProduct r;
SExpr => OpSub(pos, l, r) : SExpr l, "-", SProduct r;
SExpr => x : SProduct x;

Expr SProduct();
SProduct => OpMul(pos, l, r) : SProduct l, "\*", SAtom r;
SProduct => OpDiv(pos, l, r) : SProduct l, "/", SAtom r;
SProduct => x : SAtom x;

Expr SAtom();
SAtom => Literal(pos, num) : "-?[0-9]+" num;
SAtom => x : "(", SExpr x, ")";
```

Also, the classes that implement the operators look like below. Note that only the name and the last
instruction in the `code` function differ:

```bs
class OpAdd extends Expr {
    private Expr lhs;
    private Expr rhs;

    init(SrcPos pos, Expr lhs, Expr rhs) {
        init(pos) {
            lhs = lhs;
            rhs = rhs;
        }
    }

	void code(CodeGen to) : override {
		Var tmp = to.l.createIntVar(to.block);
		rhs.code(to);
		to.l << mov(tmp, eax);
		lhs.code(to);
		to.l << add(eax, tmp);
	}
}

class OpSub extends Expr {
    private Expr lhs;
    private Expr rhs;

    init(SrcPos pos, Expr lhs, Expr rhs) {
        init(pos) {
            lhs = lhs;
            rhs = rhs;
        }
    }

	void code(CodeGen to) : override {
		Var tmp = to.l.createIntVar(to.block);
		rhs.code(to);
		to.l << mov(tmp, eax);
		lhs.code(to);
		to.l << sub(eax, tmp);
	}
}

class OpMul extends Expr {
    private Expr lhs;
    private Expr rhs;

    init(SrcPos pos, Expr lhs, Expr rhs) {
        init(pos) {
            lhs = lhs;
            rhs = rhs;
        }
    }

	void code(CodeGen to) : override {
		Var tmp = to.l.createIntVar(to.block);
		rhs.code(to);
		to.l << mov(tmp, eax);
		lhs.code(to);
		to.l << mul(eax, tmp);
	}
}

class OpDiv extends Expr {
    private Expr lhs;
    private Expr rhs;

    init(SrcPos pos, Expr lhs, Expr rhs) {
        init(pos) {
            lhs = lhs;
            rhs = rhs;
        }
    }

	void code(CodeGen to) : override {
		Var tmp = to.l.createIntVar(to.block);
		rhs.code(to);
		to.l << mov(tmp, eax);
		lhs.code(to);
		to.l << idiv(eax, tmp);
	}
}
```

Variables
---------

Function Calls
--------------


Syntax Highlighting
-------------------