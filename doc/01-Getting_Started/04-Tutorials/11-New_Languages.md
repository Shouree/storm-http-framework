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
│
├─ demo
│  ├─ ast.bs
│  ├─ demo.bs
│  └─ syntax.bnf
└─ test
   ├─ demo.demo
   └─ test.bs
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

Now that we can evaluate expressions properly, let's make the language more useful by adding
variables. Since there are no assignments in the language, the only way to define a variable is to
specify it as a parameter to a function.

Under these constraints, it would be possible to implement variables trivially using a simple hash
table. This tutorial will, however, illustrate a slightly more complex implementation that is able
to accommodate more advanced languages as well. In particular, the implementation here will utilize
the name lookup mechanisms in Storm, so that it is possible to look up variables and functions using
an uniform interface. The structure also allows implementing things like blocks that introduce
separate namespaces for example. All in all, the structure is a simplified version of the structure
used in Basic Storm.

As in the previous parts of the tutorial, we start by updating our test file. To test parameters and
variables, let's add a new test to the `demo.demo` file, so that it looks like below:

```
f() = (10 + 4) / 2
f(a, b) = a + b + 1
```

If we run the program now, it will actually still work. This is due to the lazy compilation in
Storm. The file will parse successfully since we added a production that matches variables. This
means that the code parses correctly, even if we have not yet implemented support for variables.
Since we never call `f` that accepts two parameters, we will never try to compile it, and we thus
never discover the error. As mentioned [here](md:../Developing_in_Storm/Compilation_Model), we can
force Storm to compile all of our code eagerly to see the error. This does, however, mean that we
have to implement function calls as well before being able to test our implementation of variables.
The easiest way to test our implementation is to modify the `main` function in the `test.bs` file to
call the new function:

```bs
void main() {
    Int result = f(1, 2);
    print("Result: ${result}");
}
```


If we run the program at this stage, we get an error as we would expect:

```
@/home/storm/language/demo/syntax.bnf(962-981): Syntax error:
No return value specified for a production that does not return 'void'.
```

As before, we solve this error by adding a transform function to the production. This time we use it
to create a node called `VarAccess`:

```bnf
SAtom => VarAccess(pos, name) : SIdentifier name;
```

We also need to define the `VarAccess` class in the `ast.bs` file as before:

```bs
class VarAccess extends Expr {
    init(SrcPos pos, Str name) {
        init(pos) {}
    }

    void code(CodeGen to) : override {
        // ?
    }
}
```

This is enough to allow us to run the program, but not enough to produce the correct result (I get
the number 3 when running it, but it might differ on your machine or even crash). The big question
that remains is: how do we know which variable to read in the `code` function?

To answer that question, let's start at the root of the AST, when we start code generation in the
`createCode` function inside the `DemoFunction` class. At the moment we completely ignore that the
function receives parameters. This means that we actually generate code for a function that accepts
0 parameters, even though we pass two integers when we call it. The first step is therefore to tell
the `Listing` object that the function accepts parameters, so that it can generate code that
receives the parameters properly. We do this by calling the `createParam` function for each
parameter, and giving it a `TypeDesc` (type description) that contains information about the type
that should be received. We can easily get a `TypeDesc` from a `Type` by calling its `desc()`
member:

```bsclass:use=lang.bs.macro
private CodeGen createCode() {
    CodeGen gen(runOn, isMember, result);

    gen.l << prolog();

    for (name in paramNames) {
        Var param = gen.l.createParam(named{Int}.desc());
    }

    Expr e = body.transform();
    e.code(gen);

    gen.l << fnRet(eax);

    return gen;
}
```

As we can see from the code, the `createParam` function returns a `Var` object. This means that we
can treat parameters in the Intermediate Language just as we treat normal variables. This means that
as long as we manage to get the `Var` values from the `createCode` function to the `VarAccess`
class, we can easily generate code to load the variable into the `eax` register. The question that
remains is therefore: how do we allow the `VarAccess` class to access the parameters?

As mentioned above, we will solve name lookups using the facilities provided by Storm. As such, the
first step is to create a named entity for storing our variables. We can inherit from the
[stormname:core.lang.Variable] class in Storm to simplify the implementation a bit:

```bsclass:use=lang.bs.macro
class DemoVar extends Variable {
    Var codeVar;

    init(SrcPos pos, Str name, Var codeVar) {
       init(pos, name, named{Int}) {
           codeVar = codeVar;
       }
    }
}
```

As we can see, this class simply stores a `Var` instance that corresponds to the variable we
received from the `createParam` function. The base class `Variable` also gives it a `result` member
that stores the type of the variable (this is why we pass `named{Int}` to the parent constructor).
Furthermore, since `Variable` inherits from `Named`, it also has a name and a source position. The
inheritance from `Named` also means that it can be a part of the name tree in Storm, and that we can
search for it using the standard lookup mechanisms.

Before we can use the `DemoVar` class we just created, we need somewhere to store them. Since the
variables are local to the function, we don't want them to be accessible from the root of the name
tree. We do, however, wish to have a node in the name tree with a parent pointer to the right point
in the name tree, so that we can lookup functions and other variables later on. For this reason, we
create a class that inherits from [stormname:core.lang.NameLookup]. The `NameLookup` class is the
super class of [stormname:core.lang.Named], and represents a node in the name tree where it is
possible to look up names, and that has a parent pointer to some location in the name tree. It does,
however, not have a name and it is therefore not possible to store a `NameLookup` in the name tree,
or to find them by name. This is exactly what we want. We start the implementation as follows:

```bs
class DemoLookup extends NameLookup {
    init(NameLookup parent) {
        init(parent) {}
    }
}
```

Since we wish to be able to look up names in the name tree from the `NameLookup` later on, we let
the constructor accepts a parameter that specifies the parent node in the name tree. This also lets
us link multiple `DemoLookup` together to represent nested namespaces inside functions in our
language if we wish.

The next step is to add somewhere to store variables inside the `DemoLookup`. Since variables do not
have any parameters, we can simply use a `Map<Str, DemoVar>` (also spelled `Str->DemoVar`). We also
add a function that makes it convenient to add variables:

```bs
class DemoLookup extends NameLookup {
    Str->DemoVar variables;

    init(NameLookup parent) {
        init(parent) {}
    }

    void addVar(SrcPos pos, Str name, Var v) {
        DemoVar var(pos, name, v);
        var.parentLookup = this;
        variables.put(name, var);
    }
}
```

One thing is worth noting in the code above. In the `addVar` function we explicitly set
`parentLookup` of the created variable to `this`. This connects the variable to the current block,
so that it is possible to start in the variable and traverse the name tree towards the root. We do
not need this ability in our language, but for some types of named entities (e.g. functions), it is
necessary to attach them to the name tree before using them. The function `add` in the `NameSet`
class sets the `parentLookup` internally, so it is only necessary to set `parentLookup` manually when
working with `NameLookups` like in this example.

We can now go back to the `createCode` function and store the parameters inside a `DemoLookup`:

```bsclass:use=lang.bs.macro
private CodeGen createCode() {
    CodeGen gen(runOn, isMember, result);

    gen.l << prolog();

    DemoLookup lookup(this);
    for (name in paramNames) {
        Var param = gen.l.createParam(named{Int}.desc());
        lookup.addVar(pos, name, param);
    }

    Expr e = body.transform();
    e.code(gen);

    gen.l << fnRet(eax);

    return gen;
}
```

Now that we have stored the parameters somewhere, we just need to pass them to the `VarLookup` class
through the grammar. We do this by encapsulating the `DemoLookup` inside a
[stormname:core.lang.Scope] class. The `Scope` class can be thought of as a pointer to a location in
the name tree, together with a strategy for looking up names based on that location. For example,
when asked to look up the name `Str` in the context of the package `lang.demo`, the `Scope` used in
Basic Storm will first look in the package `lang.demo`, then in the root package, and after that in
the `core` package (the strategy is customizable by providing a `ScopeLookup` class, but we will not
need to do that in this tutorial).

To give the name tree access to the current scope, we modify the call to `transform` into the
following:

```bsstmt
Expr e = body.transform(Scope(lookup));
```

We must also change the syntax to accept the new parameter and pass it along to the `VarAccess`
class. We first add `use core.lang;` to the `syntax.bnf` file so we do not have to write
`core.lang.Scope` every time we wish to refer to the `Scope` class. After that, we add a parameter
`scope` with the type `Scope` to the `SExpr`, `SProduct`, and `SAtom` rules, and modify the usages
of these rules to pass the `scope` parameter along. After the changes, the productions should look
like this:

```bnf
use core.lang;

// ...

Expr SExpr(Scope scope);
SExpr => OpAdd(pos, l, r) : SExpr(scope) l, "\+", SProduct(scope) r;
SExpr => OpSub(pos, l, r) : SExpr(scope) l, "-", SProduct(scope) r;
SExpr => x : SProduct(scope) x;

Expr SProduct(Scope scope);
SProduct => OpMul(pos, l, r) : SProduct(scope) l, "\*", SAtom(scope) r;
SProduct => OpDiv(pos, l, r) : SProduct(scope) l, "/", SAtom(scope) r;
SProduct => x : SAtom(scope) x;

Expr SAtom(Scope scope);
SAtom => Literal(pos, num) : "-?[0-9]+" num;
SAtom => x : "(", SExpr(scope) x, ")";

SAtom => VarAccess(pos, name, scope) : SIdentifier name;
```

Now, we can finally return to our `VarAccess` implementation. We start by modifying the constructor
to accept the new `Scope` parameter. It can then use the new parameter to try to look up the
variable as follows:

```bsclass
init(SrcPos pos, Str name, Scope scope) {
    init(pos) {}

    if (x = scope.find(SimpleName(name)) as DemoVar) {
        print("Found ${x.name}!");
    } else {
        throw SyntaxError(pos, "Unknown variable: ${name}");
    }
}
```

As we can see, the `Scope` class has a `find` function that accepts a single parameter that contains
the name that it should attempt to find. Since we have no unresolved parameters, we can simply
create a [stormname:core.lang.SimpleName] that consists of a single part that contains the name we
are looking for. The function returns a `Maybe<Named>` value that is `null` if no name was found.
Since we are looking for `DemoVar` instances, we cast it to a `DemoVar` using the `as` keyword. This
check is also able to check for `null` at the same time. For now, we simply print a message if we
found a matching variable, and throw a syntax error on failure.

If we run the code like this, we will (perhaps surprisingly) reach the error case:

```
@/home/storm/language/test/demo.demo(29-30): Syntax error: Unknown variable: a
```

To understand why the error occurs, we need to know a bit more about how the `find` function works
internally. On a high level, the `find` function iterates through the individual parts in the
supplied name. It attempts to find the first part in the `NameLookup` stored in the `Scope`. The
next part is resolved in the context of the entity found by the first, and so on. To find a single
part in a `NameLookup` class, the `Scope` calls the `find` function in the `NameLookup`. If it fails
by returning `null`, the name lookup is considered a failure, and the `Scope` may try again from a
different starting point (e.g. the parent to the current `NameLookup`).

Knowing this strategy allows us to find the problem in our implementation. While we store `DemoVar`
entities in our `DemoLookup`, we have not yet implemented the `find` function that lets the `Scope`
find the variables. As such, we need to take a slight detour and override the `find` function in our
`DemoLookup` class:

```bsclass
Named? find(SimplePart part, Scope scope) : override {
    if (part.params.empty) {
        if (found = variables.at(part.name)) {
            return found;
        }
    }
    return null;
}
```

The function receives two parameters. The first one is the part of the name that the `Scope` wishes
to find. The second parameter is the scope itself. It can be used to inspect the context in which
the name lookup occurred, so that we can determine which variables are visible from that context
(e.g. for private variables). Since we are working with local variables that are not reachable from
anywhere outside the function, we can safely ignore the `scope` parameter in our implementation.

The `find` function starts by checking if the `part` contains any parameters. If it does, we know
that none of our variables will match, since variables do not take parameters by definition. If the
part had no parameters, we can proceed to look up the variable in the hash table, and return it if
we found a match.

This is enough for our needs, and if we try to run our program again, we can see that the error
message is replaced by the messages `Found a!` and `Found b!` This means that our name lookup works
as expected. The only thing that remains is to use the found entity to retrive the value of the
variable!

We add a variable in the `VarAccess` class that stores the `DemoVar` we found in the constructor.
Note, however, that due to how the `unless` statement is implemented in Basic Storm, it is not
possible to use it before `init` or `super` blocks. This means that we need to rely on the fact that
`if` statements are expressions when we initialize the `DemoVar` variable (it is possible to write a
helper function as well, but where is the fun in that?). Once we have stored the `DemoVar` in the
class, we can use it in the `code` function to load the `codeVar` into the register `eax`:

```bs
class VarAccess extends Expr {
    DemoVar toRead;

    init(SrcPos pos, Str name, Scope scope) {
        init(pos) {
            toRead = if (x = scope.find(SimpleName(name)) as DemoVar) {
                x;
            } else {
                throw SyntaxError(pos, "Unknown variable: ${name}");
            };
        }
    }

    void code(CodeGen to) : override {
        to.l << mov(eax, toRead.codeVar);
    }
}
```

And with those changes, the implementation of variables is complete. If we run the program now, it
will print `Result: 4` as expected.


Function Calls
--------------

After implementing variables, the last thing that remains is to implement function calls. Luckily,
it turns out that we can re-use much of the work we did to implement variables when implementing
functions. In particular, we can use the `find` function in the `Scope` class to look for functions
as well.

Before we get to the implementation, let's update our `demo.demo` file with some new tests:

```
f() = (10 + 4) / 2
f(a, b) = a + b + 1
g(a, b) = f(a, a) * b
```

We also change the `main` function in `test.bs` as follows:

```bs
void main() {
    print("f(1, 2) = ${f(1, 2)}");
    print("g(4, 2) = ${g(4, 2)}");
}
```

If we run the program at this point, we get the familiar error message below that indicates that we
are trying to transform a production that we have not yet specified a transform for:

```
@/home/storm/language/demo/syntax.bnf(1145-1184): Syntax error:
No return value specified for a production that does not return 'void'.
```

To fix the issue, we need to add transforms to the function call production. Since it uses the
`SActuals` rule, we also need to update that production to include transforms and the `Scope`
parameter:

```bnf
SAtom => FnCall(pos, name, params, scope) : SIdentifier name, "(", SActuals(scope) params, ")";

Array<Expr> SActuals(Scope scope);
SActuals => Array<Expr>() : ;
SActuals => Array<Expr>() : SExpr(scope) -> push - (, ",", SExpr(scope) -> push)*;
```

Next, we need to implement the `FnCall` node in the file `ast.bs`. We start with the basic
structure:

```bs
class FnCall extends Expr {
    private Expr[] params;

    init(SrcPos pos, Str name, Expr[] params, Scope scope) {
        init(pos) {
            params = params;
        }
    }

    void code(CodeGen to) : override {
        // ?
    }
}
```

As was the case for variables, it is now possible to run the test. It will run and produce some
answer (4 on my system, but it may differ on your system). This is nice as it lets us experiment a
bit easier.

The implementation in the `FnCall` class will be similar to the `VarAccess` class. In the
constructor we will use the `scope` to find the function to call. We store it as a variable in the
class so that we can use it in the `code` function. The difference is that we need to create a name
that has the right number of integer parameters. Otherwise we will not find the function we are
looking for. Since we will be able to find functions in other languages in Storm, it is also a good
idea to verify the return type of the function we found.

We start by creating a name to look for in the constructor:

```bsclass:use=lang.bs.macro
init(SrcPos pos, Str name, Expr[] params, Scope scope) {
    SimplePart part(name);
    for (x in params)
        part.params.push(named{Int});

    SimpleName lookFor(part);

    // For debugging!
    print(lookFor.toS);

    init(pos) {
        params = params;
    }
}
```

We start by creating a `SimplePart` that only contains the name of the function we are looking for.
We then add one integer parameter for each parameter that was passed to the function. The reason we
do not have to inspect the expressions in the `params` array is again since the `demo` language only
supports integers. This means that we know that all expressions evaluate to `Int`. If we add support
for other types, we would have to ask each parameter what type it evaluated to and add that type to
the `part` instead.

After we have created the `part`, we create a `SimpleName` out of the part. In our case, the name
only contains a single part since the `demo` language does not have any syntax to specify functions
that are located in other packages (e.g. `tutorials.language.main()`). Finally, we print the name to
see that we have created something that is sensible. If we run the program it will print
`f(core.Int, core.Int)` which looks exactly like what we would expect.

Since the created name looks right, we can try to look up the name using the `scope`. We add a new
variable `toCall` to store the function in, and like in the `VarAccess` class, we will initialize it
using an `if` statement in the `init` block:

```bsclass:use=lang.bs.macro
class FnCall extends Expr {
    private Expr[] params;
    private Function toCall;

    init(SrcPos pos, Str name, Expr[] params, Scope scope) {
        SimplePart part(name);
        for (x in params)
            part.params.push(named{Int});

        SimpleName lookFor(part);

        init(pos) {
            params = params;
            toCall = if (x = scope.find(lookFor) as Function) {
                if (!Value(named{Int}).mayStore(x.result))
                    throw SyntaxError(pos, "Functions used in the demo language must return Int!");
                x;
            } else {
                throw SyntaxError(pos, "Unknown function: ${lookFor}");
            };
        }
    }

    // ...
}
```

As can be seen above, the only difference from the `VarAccess` class is that we try to cast the
found element to a `Function` rather than a `DemoVar`, and that we also verify the result using the
`mayStore` function in the `Value` class. This function checks that the left hand side (the `Int`)
is a binary compatible with the right hand side (`x.result`). For value types, it would technically
be enough to check if the type is exactly `Int`. The `mayStore` function takes inheritance for
class- and actor-types as well, which is usually useful but not really necessary here.

Finally, the only thing that remains is to implement the `code` function. Here, we can take a
similar approach to the implementation of the `code` functions for the operators. We start by
evaluating all parameters to the function, and store them in temporary variables, and then call the
function. We can evaluate the parameters and store them in variables like this:

```bsclass
void code(CodeGen to) : override {
    Operand[] ops;
    for (x in params) {
        Var tmp = to.l.createIntVar(to.block);

        s.code(to);
        to.l << mov(tmp, eax);
        ops << tmp;
    }
}
```

We then have two options to call the function. The most robust one is to utilize the `autoCall`
function inside the `Function` class. The `autoCall` function inspects the `CodeGen` object and
determines if a thread switch is necessary or not. Using it requires creating a `CodeResult` object
to let it know where it should place the result. This can be done as follows:

```bsclass:use=lang.bs.macro
void code(CodeGen to) : override {
    Operand[] ops;
    for (x in params) {
        Var tmp = to.l.createIntVar(to.block);

        x.code(to);
        to.l << mov(tmp, eax);
        ops << tmp;
    }

    CodeResult result(named{Int}, to.block);
    toCall.autoCall(to, ops, result);
    to.l << mov(eax, result.location(to));
}
```

In the first new line, we create a `CodeResult` and specify that we want the result to be located in
a variable visible in the current block (`to.block`), and that the variable should store an integer.
Then, we call `autoCall` to ask it to generate code for the function call. Finally, we copy the
resultfrom the variable the `CodeResult` object created for it to the `eax` register to follow the
convention of the `code` function.

The other option is to emit `fnParam` and `fnCall` instructions directly. This works in cases where
we are sure that a thread switch is *not* necessary. We can not really assume this in our
implementation. Therefore, the following implementation is *not* recommended in this case. It is
only provided to illustrate how function calls in the intermediate language work:

```bsstmt:use=lang.bs.macro
for (x in ops) {
    to.l << fnParam(named{Int}.desc, x);
}
to.l << fnCall(toCall.ref(), toCall.isMember, named{Int}.desc, x);
```

With that, the implementation of function calls is complete. Running the tests now should produce
the expected output:

```
f(1, 2) = 4
f(4, 2) = 18
```

Since we are using the generic `Function` interface for function calls, it is actually possible to
call functions implemented in Basic Storm from our `demo` language. To illustrate this, we can
create a new function in the file `demo.demo`:

```
h(a) = bsfn(f(a, a), 5)
```

We then update our `main` function in the file `test.bs` to call the new function. We also implement
`bsfn` there:

```bs
void main() {
    print("f(1, 2) = ${f(1, 2)}");
    print("g(4, 2) = ${g(4, 2)}");
    print("h(10) = ${h(10)}");
}

Int bsfn(Int x, Int y) {
    print("In Basic Storm: ${x}, ${y}");
    x + y;
}
```

If we run the program now, we can see that it works as expected. The output from the `print`
statement in `bsfn` is visible as we would expect, and the correct result is produced:

```
f(1, 2) = 4
g(4, 2) = 18
In Basic Storm: 21, 5
h(10) = 26
```


Syntax Highlighting
-------------------

Using the [language server](md:/Language_Reference/Storm/Language_Server) in Storm, we can easily
add syntax higlighting to our language. For this, we need two things: a function in our `FileReader`
that tells the language server which rule is the root in our grammar, and annotations in the
grammar.

We start by adding the required function to the `FileReader`. In the class `DemoFileReader`, we
simply need to add the following function:

```bsclass:use=lang.bs.macro
lang:bnf:Rule rootRule() : override {
    named{SRoot};
}
```

This instructs the language server to start parsing `demo` files using the rule `SRoot`. Apart from
that, we just need to annotate the grammar to tell the language server which parts of the text to
highlight, and which color to use.

To allow the end-user to customize the appearance of the highlighted code, the colors for tokens are
expressed in terms of what function the characters have in the source code. The following classes
are available:

- `#comment` - for comments
- `#delimiter` - for delimiters (e.g. punctuation)
- `#string` - for string literals
- `#constant` - for numeric literals (i.e. constant numbers)
- `#keyword` - for keywords
- `#fnName` - for function names
- `#varName` - for variable names
- `#typeName` - for names of types

Of course, Storm places no requirement on a language to highlight all parts of the text.
Furthermore, some languages might make it difficult to distinguish between for example type- and
variable names from the information available to the parser. In such cases, it might be possible to
modify the grammar slightly to match conventions in the language. For example, it is possible to
split a production that matches a generic identifier into two. One matches identifiers that start
with an uppercase letter, and is highlighted as a type name. Another matches identifiers that start
with a lowercase letter, and is highlighted as a function name. Since the rules can still contain
the same syntax transforms, this does not impact the rest of the language.

With this in mind, we can then simply annotate our syntax by adding `#`-annotations after the
relevant tokens. This ends up looking like below:

```bnf
Array<DemoFunction> SRoot();
SRoot => functions : , (SFunction functions, )*;

DemoFunction SFunction();
SFunction => DemoFunction(pos, name, params, body)
  : SIdentifier name #fnName, "(", SParamList params, ")", "=", SExpr @body;

Str SIdentifier();
SIdentifier => x : "[A-Za-z]+" x;

Array<Str> SParamList();
SParamList => Array<Str>() : ;
SParamList => Array<Str>()
  : SIdentifier -> push #varName - (, ",", SIdentifier -> push #varName)*;

Expr SExpr(Scope scope);
SExpr => OpAdd(pos, l, r) : SExpr(scope) l, "\+", SProduct(scope) r;
SExpr => OpSub(pos, l, r) : SExpr(scope) l, "-", SProduct(scope) r;
SExpr => p : SProduct(scope) p;

Expr SProduct(Scope scope);
SProduct => OpMul(pos, l, r) : SProduct(scope) l, "\*", SAtom(scope) r;
SProduct => OpDiv(pos, l, r) : SProduct(scope) l, "/", SAtom(scope) r;
SProduct => a : SAtom(scope) a;

Expr SAtom(Scope scope);
SAtom => Literal(pos, num) : "-?[0-9]+" num #constant;
SAtom => VarAccess(pos, name, scope)  : SIdentifier name #varName;
SAtom => FnCall(pos, name, params, scope)
  : SIdentifier name #fnName, "(", SActuals(scope) params, ")";
SAtom => x : "(", SExpr(scope) x, ")";

Array<Expr> SActuals(Scope scope);
SActuals => Array<Expr>() : ;
SActuals => Array<Expr>() : SExpr(scope) -> push - (, ",", SExpr(scope) -> push)*;
```

In order to test the syntax highlighting, it is necessary to start the [language
server](md:../Developing_in_Storm/Emacs_Integration). If you have developed the new language
directly in Storm's `root/` directory, then you do not need to do anything else. If you developed it
separately, you need to make sure that the library is properly included in the Storm process used as
the language server. This is done by modifying the variable `storm-mode-include` in Emacs before
starting the language server. A good way to do this short-term is to simly set the variable to the
value we want using `setq`. This makes it easier to re-set the variable if we make a mistake:

```
(setq storm-mode-include '(("/path/to/demo" . "lang.demo")))
```

One way to evaluate the expression is to press `M-x`, type `eval-expression`, press Enter, and then
typing the expression above and finish by hitting Enter. Another way is to open the buffer
`*scratch*`, type the expression there, and then press `C-M-x` at the end of the expression.



Ideas for Extending the Language
--------------------------------

The language created here provides a good starting point for further development. Below are some
ideas for how to expand the language further, towards a more "real" language:

- **Assignments**

  Based on what is currently in the language, it is fairly straight-forward to add assignments
  (perhaps in the form of `let` statements) to the language. It is already possible to add new
  variables to the `DemoLookup` object. The tricky part is that the variables must be added in the
  constructor of the AST node that creates them, but the `codeVar` variable can not be initialized
  until code is generated. Fortunately, the `Var` type has an empty constructor that creates an
  invalid variable value that can be used as a placeholder.

  It is also interesting to further extend the language by introducing blocks that delimit the scope
  of local variables. This can be done by representing the new scope as a new instance of the
  `DemoLookup` class. By setting the `parent` to the previous `DemoLookup` instance, the `Scope`
  will actually traverse the scopes as we would expect by default.

- **Extensible Syntax**

  Since the parser in Storm is built to support extensible languages, it is quite easy to use this
  functionality in our demo language. The goal is to call the function `addSyntax` on the parser to
  add new packages with syntax before we call `parse`. The question is how we manage to read which
  packages to include before we start parsing anything.

  The key to this problem is that it is possible to divide files into different sections by chaining
  `FileReader`s. This allows us to treat a file in our language like this:

  ```
  use my.package
  ---------------
  f(a) = 10
  ```

  Where the dashed line represents how we "split" the file into different parts. The two parts are
  essentially treated as different languages. The first one is only able to parse `use` statements,
  and the second only knows the function definition syntax. We also implement the first reader so
  that it does not produce an error if it is not able to reach the end of the file. This can be done
  by simply replacing `parser.hasError` with `!parser.hasTree` in the code.

  In summary, the structure would look approximately like this:

  ```bs
  lang:PkgReader reader(Url[] files, Package pkg) on Compiler {
      lang:filePkgReader(files, pkg, (x) => DemoIncludeReader(x));
  }

  class DemoIncludeReader extends lang:FileReader {
      init(lang:FileInfo info) {
          init(info) {}
      }

      // We can provide 'readX' functions as well if we wish to. If we do, it is
      // a good idea to make sure that we only parse the text once, as parsing is
      // a fairly expensive operation.

      // Called by the system to request the "next" piece.
      lang:FileReader? next(lang:ReaderQuery query) : override {
          Parser<SIncludes> parser;
          parser.parse(info.contents, info.url, info.start);
          if (!parser.hasTree())
              throw parser.error();

          // Extract use-statements here.
          Name usedPkgs = [];

          // Create the "normal" reader. Ask it to start reading where we stopped.
          // We can pass any new parameters as well.
          return DemoFileReader(info.next(parser.matchEnd), usedPkgs);
      }
  }

  class DemoFileReader extends lang:FileReader {
      Name[] used;

      init(lang:FileInfo info, Name[] used) {
          init(info) {
              used = used;
          }
      }

      void readFunctions() : override {
          Parser<SRoot> parser;
          for (x in used) {
              // Resolve 'x'
              parser.addSyntax(resolved);
          }
          parser.parse(info.contents, info.url, info.start);
          // ...
      }
  }
  ```

- **Types**

  It requires some work to extend the language to other types, but it is doable. This requires
  adding a member to the `Expr` class that returns the type of each expression. This type
  information can then be used in the function call node to find a function that accepts the proper
  parameters. Since we can no longer guarantee that the result of evaluating a node fits in a
  register anymore, we need to transition to another scheme. One option is to let the `code`
  function return a `Var` that contains the value. This results in quite a number of copies of value
  types, but works. Copies can be eliminated by letting nodes work with references to value types,
  for example.


Full Source Code
----------------

For completeness, the full source produced in this tutorial is provided below.

`syntax.bnf`:

```bnf
use core.lang;

optional delimiter = SDelimiter;

void SDelimiter();
SDelimiter : "[ \n\r\t]*" - (SComment - SDelimiter)?;

void SComment();
SComment : "//[^\n]*\n" #comment;

Array<DemoFunction> SRoot();
SRoot => functions : , (SFunction functions, )*;

DemoFunction SFunction();
SFunction => DemoFunction(pos, name, params, body)
  : SIdentifier name #fnName, "(", SParamList params, ")", "=", SExpr @body;

Str SIdentifier();
SIdentifier => x : "[A-Za-z]+" x;

Array<Str> SParamList();
SParamList => Array<Str>() : ;
SParamList => Array<Str>()
  : SIdentifier -> push #varName - (, ",", SIdentifier -> push #varName)*;

Expr SExpr(Scope scope);
SExpr => OpAdd(pos, l, r) : SExpr(scope) l, "\+", SProduct(scope) r;
SExpr => OpSub(pos, l, r) : SExpr(scope) l, "-", SProduct(scope) r;
SExpr => p : SProduct(scope) p;

Expr SProduct(Scope scope);
SProduct => OpMul(pos, l, r) : SProduct(scope) l, "\*", SAtom(scope) r;
SProduct => OpDiv(pos, l, r) : SProduct(scope) l, "/", SAtom(scope) r;
SProduct => a : SAtom(scope) a;

Expr SAtom(Scope scope);
SAtom => Literal(pos, num) : "-?[0-9]+" num #constant;
SAtom => VarAccess(pos, name, scope)  : SIdentifier name #varName;
SAtom => FnCall(pos, name, params, scope)
  : SIdentifier name #fnName, "(", SActuals(scope) params, ")";
SAtom => x : "(", SExpr(scope) x, ")";

Array<Expr> SActuals(Scope scope);
SActuals => Array<Expr>() : ;
SActuals => Array<Expr>() : SExpr(scope) -> push - (, ",", SExpr(scope) -> push)*;
```

`demo.bs`:

```bs
use core:io;
use core:lang;
use lang:bs:macro;
use core:asm;

lang:PkgReader reader(Url[] files, Package pkg) on Compiler {
	lang:FilePkgReader(files, pkg, (x) => DemoFileReader(x));
}

class DemoFileReader extends lang:FileReader {
    init(lang:FileInfo info) {
        init(info) {}
    }

	void readFunctions() : override {
		Parser<SRoot> parser;
		parser.parse(info.contents, info.url, info.start);
		if (parser.hasError)
			throw parser.error();

		DemoFunction[] functions = parser.tree().transform();
		for (f in functions)
			info.pkg.add(f);
	}
}


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

	private CodeGen createCode() {
		CodeGen gen(runOn, isMember, result);

		gen.l << prolog();

		DemoLookup lookup(this);
		for (name in paramNames) {
			Var param = gen.l.createParam(named{Int}.desc());
			lookup.addVar(pos, name, param);
		}

		Expr e = body.transform(Scope(lookup));
		e.code(gen);

		gen.l << fnRet(eax);

		return gen;
	}
}


class DemoVar extends Variable {
    Var codeVar;

    init(SrcPos pos, Str name, Var codeVar) {
       init(pos, name, named{Int}) {
           codeVar = codeVar;
       }
    }
}

class DemoLookup extends NameLookup {
    Str->DemoVar variables;

    init(NameLookup parent) {
        init(parent) {}
    }

    void addVar(SrcPos pos, Str name, Var v) {
        DemoVar var(pos, name, v);
        var.parentLookup = this;
        variables.put(name, var);
    }

	Named? find(SimplePart part, Scope scope) : override {
		if (part.params.empty) {
			if (found = variables.at(part.name)) {
				return found;
			}
		}
		return null;
	}
}
```

`ast.bs`:

```bs
use core:asm;
use core:lang;
use lang:bs:macro;

class Expr on Compiler {
	SrcPos pos;

	init(SrcPos pos) {
		init { pos = pos; }
	}

	// Convention: Output result into the eax register.
	void code(CodeGen to) : abstract;
}

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

class VarAccess extends Expr {
    DemoVar toRead;

    init(SrcPos pos, Str name, Scope scope) {
        init(pos) {
            toRead = if (x = scope.find(SimpleName(name)) as DemoVar) {
                x;
            } else {
                throw SyntaxError(pos, "Unknown variable: ${name}");
            };
        }
    }

    void code(CodeGen to) : override {
        to.l << mov(eax, toRead.codeVar);
    }
}

class FnCall extends Expr {
    private Expr[] params;
    private Function toCall;

    init(SrcPos pos, Str name, Expr[] params, Scope scope) {
        SimplePart part(name);
        for (x in params)
            part.params.push(named{Int});

        SimpleName lookFor(part);

        init(pos) {
            params = params;
            toCall = if (x = scope.find(lookFor) as Function) {
                if (!Value(named{Int}).mayStore(x.result))
                    throw SyntaxError(pos, "Functions used in the demo language must return Int!");
                x;
            } else {
                throw SyntaxError(pos, "Unknown function: ${lookFor}");
            };
        }
    }

    void code(CodeGen to) : override {
		Operand[] ops;
		for (x in params) {
			Var tmp = to.l.createIntVar(to.block);

			x.code(to);
			to.l << mov(tmp, eax);
			ops << tmp;
		}

		CodeResult result(named{Int}, to.block);
		toCall.autoCall(to, ops, result);
		to.l << mov(eax, result.location(to));
    }
}

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


`test.bs`:

```bs
void main() {
    print("f(1, 2) = ${f(1, 2)}");
    print("g(4, 2) = ${g(4, 2)}");
	print("h(10) = ${h(10)}");
}

Int bsfn(Int x, Int y) {
	print("In Basic Storm: ${x}, ${y}");
	x + y;
}
```

`demo.demo`:

```
f() = (10 + 4) / 2
f(a, b) = a + b + 1
g(a, b) = f(a, a) * b
h(a) = bsfn(f(a, a), 5)
```
