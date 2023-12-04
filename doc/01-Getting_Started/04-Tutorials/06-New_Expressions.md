A New Expression in Basic Storm
===============================

This tutorial shows how to create syntax extensions to Basic Storm. We will implement two relatively
simple extensions. The first one is a loop-like repeat block that simply executes the loop body a
certain number of times:

```bsstmt:use=tutorials.expressions.ast
repeat(10) {
    print("Test");
}
```

The second one is a simple map-like construct (limited to integers for simplicity):

```bsstmt:use=tutorials.expressions.ast
Int[] input = [1, 2, 3];
Int[] output = map(x * 2 for x in input);
```

As with the other tutorials, the code produced by this tutorial is available in
`root/tutorials/expressions`. You can run it by typing `tutorials:expressions:main` in the Basic Storm
top loop. Note that the structure of the aforementioned directory is slightly different to what is
proposed in the tutorial. This is so that it can contain both an implementation that generates ASTs
in Basic Storm, and an implementation that generates code in the intermediate representation
directly.


Setup
-----

First, we need somewhere to work. Create an empty directory somewhere on your system. The name does
not matter too much, but the remainder of the tutorial assumes that it is `expressions`. If you pick
another name, you need to modify the package names in `use` statements.

Create two subdirectories inside the directory you just created: `repeat` and `map`. We will place
the two extensions inside these directories. This is so that we can create a `main` function that
uses the extensions in a separate package from the extension itself. It is usually not a good idea
to define and use a language extension in the same package, as that easily creates dependency cycles
that are difficult to resolve.

Finally, create a file named `test.bs` (or some other name) inside the `expressions` directory. Start
by adding a definition of a `main` function as below:

```bs
void main() {
}
```

We will use this function to write tests later on. After all of this, open a terminal and navigate
to the newly created directory. This makes it possible to run the code we will write by typing:

```
storm .
```

If done successfully, this will not produce any output, and Storm will exit successfully.


Repeat Syntax Using the Basic Storm AST
---------------------------------------

First, we will implement the `repeat` syntax as a form of macro using the facilities already present
in Basic Storm. The first step is to create the grammar for the new syntax. To do this, create a
file `syntax.bnf` inside the directory `repeat`. Inside the file we start by using the `lang.bs`
namespace to easily access the productions in Basic Storm, and then we define the delimiters as in
the [previous](md:Using_Grammar) tutorial:

```bnf
use lang.bs;

optional delimiter = SDelimiter;
required delimiter = SRequiredDelimiter;
```

After this, we can add a production that extends the `SStmt` rule in Basic Storm. We pick the
`SStmt` rule since the `repeat` block will act as a statement (i.e. it is not necessary to put a
semicolon after it). We start simple, by adding just the syntax and saving transforms for later:

```bnf
SStmt : "repeat" #keyword, "(", SExpr, ")", SStmt;
```

This rule matches the word `repeat` (colored as a keyword), followed by a start parenthesis, an
expression that indicates the number of iterations, and end parenthesis, and a statement that
constitutes the body of the `repeat` block.

We can now write a simple test in the `test.bs` file as follows:

```bs:partial
void main() {
    repeat(5) {
        print("Hello");
    }
}
```

If you try to run the test now (with `storm .`), you will receive a parse error (complaining about
`{`). This is because we have not yet included our package, so the new syntax is not yet visible. To
include the grammar, we simply add the following to the top of the file:

```bs
use expressions:repeat;
```

If we run the test yet again, we will receive the error: "No return value specified for a production
that does not return 'void'." This means that we have not yet added a transform function to our
production. However, due to Storm's lazy compilation, this means that the rule was successfully
matched at least!

So, the next step is to add logic for the transform. We start by adding a call to a function that
creates a syntax node, and passing it the repeat count and the body of the statement:

```bnf
SStmt => repeatStmt(times, body) : "repeat" #keyword, "(", SExpr times, ")", SStmt body;
```

If we run the code now, Storm will complain that it is unable to transform a `lang.bs.Expr` with
parameters `()`. This is because we have not passed the necessary parameters to the `SExpr` and
`SStmt` rules. If we look in the Basic Storm grammar for the definition of the rules, we see that
they accept a parameter named `block` that corresponds to the block in which they are located. Since
we are a statement as well, we can simply forward our parameter of the same name. We also forward the parameter to the `repeatStmt` function, since we will need it:

```bnf
SStmt => repeatStmt(block, times, body) :
  "repeat" #keyword, "(", SExpr(block) times, ")", SStmt(block) body;
```

We try to run the code again. This time, we will receive an error stating that Storm is unable to
find `repeatStmt` with parameters `(lang.bs.Block&, lang.bs.Expr&, lang.bs.Expr&)`. This is because
we have not yet implemented the `repeatStmt` function. So let's do that. Create a file `sematics.bs`
next to the `syntax.bnf` where we created the syntax. Then we add the following:

```bs
use lang:bs;

Expr repeatStmt(Block parent, Expr times, Expr body) on Compiler {
    Expr(core:lang:SrcPos());
}
```

That is, we define a function that does nothing but return an empty expression (which we need to
give a `SrcPos`). At this point, our example will actually compile and run without errors. However,
since we have not yet implemented any semantics, our program will simply ignore the body of the
`repeat` statement altogether (the reason it works at this stage is since we don't expect our
`repeat` statement to return anything, and the no-op behavior of `Expr` works for that case).

The next step is to actually implement the behavior of our block. We do this by observing that:

```bsstmt:placeholders:use=tutorials.expressions.ast
repeat(<times>) {
    <body>;
}
```

is equivalent to:

```bsstmt:placeholders
for (Nat i = <times>; i > 0; i--) {
    <body>;
}
```

This means that we can use the
[patterns](md:/Language_Reference/Basic_Storm/Extensibility/Metaprogramming) functionality from the
`lang.bs.macro` package to create the AST nodes for us. We have already matched the placeholders
`<times>` and `<body>` in our grammar. So using a `pattern`, we can insert them into the new
structure using the `${...}` syntax:

```bs
use lang:bs;
use lang:bs:macro;

Expr repeatStmt(Block parent, Expr times, Expr body) on Compiler {
	pattern(parent) {
		for (Nat i = ${times}; i > 0; i--) {
			${body};
		}
	};
}
```

At this point, our language extension works as indented. If we run the test program now (`storm .`),
we see that the program prints `Hello` five times as intended.

It is worth noting that even though it looks like we introduce a variable `i` that is visible inside
the body of the loop, this is not the case. Name lookup in Basic Storm is performed using the
`block` that is passed to statements and expressions. Since we passed the block that corresponds to
the context outside of the generated `for`-loop to the body of the loop, it will not be able to find
any variables we generate inside the `pattern`, even though it will be placed inside the `for` loop
eventually.

It is of course possible (and sometimes even necessary) to create AST nodes manually. Inspecting the
grammar for Basic Storm can give some insights into what nodes exist, and how they are used.


Repeat Syntax Using the Intermediate Language
---------------------------------------------

In the previous approach, we created code in Basic Storm that corresponds to our new language
construct. This makes it possible for us to rely on Basic Storm's ability to generate intermediate
code for us. This would not be possible if we were trying to do things that are not possible in
Basic Storm. In these cases, it is necessary to generate the intermediate code ourselves instead.

To do this, we need to create a new node that we can embed into the Basic Storm AST. We can do this
by simply creating a new class that inherits from [stormname:lang.bs.Expr]. Since we are
implementing a repeat statement, we call it `RepeatStmt`. You can either replace the code in
`semantics.bs` with this code, or create a new file to store it in.

```bs
use lang:bs;

class RepeatStmt extends Expr {}
```

From the previous implementation we already know that we will need to store two expressions. One for
the number of times the body shall be repeated, and another that contains the actual body to be
executed. It turns out that each `Expr` also contains a `SrcPos` object that stores the source
location of the node, so we must accept a `SrcPos` parameter to the constructor as well:

```bs
use lang:bs;
use lang:bs:macro;
use core:lang;
use core:asm;

class RepeatStmt extends Expr {
    private Expr times;
    private Expr body;

    init(SrcPos pos, Expr times, Expr body) {
        init(pos) {
            times = times;
            body = body;
        }
    }
}
```

To use our new class, we must also update our grammar so that it uses the class. At this time, we
can simply replace `repeatStmt(block, times, body)` with `RepeatStmt(pos, times, body)`:

```bnf
SStmt => RepeatStmt(pos, times, body) :
  "repeat" #keyword, "(", SExpr(block) times, ")", SStmt(block) body;
```

At this point it should be possible to run the program without issues (`storm .`). Again, this is
because the default implementation of `Expr` happens to do the right thing for the program to not
crash. However, we can see that the body of the `repeat` statement is not executed as we would
expect.

To fix this issue, we need to actually generate some code. This is achieved by overriding the `code`
method in the `Expr` class. The `code` function receives two parameters. The first is a
[stormname:core.lang.CodeGen] that contains the current state of the code generation, and the second
is a [stormname:core.lang.CodeResult] that is used to communicate where the result of the expression
is stored. Since our `repeat` block does not return anything, we can simply ignore the `CodeResult`
parameter for now. The `CodeGen` object is, however, more interesting. This object contains three
data members:

- `l` - a [stormname:core.asm.Listing] object we are expected to append new instructions to.

- `block` - a [stormname:core.asm.Block] that indicates which `Block` inside the `Listing` is the
  currently active block. We need this information if we wish to create new temporary variables, so
  that we can place them in the right scope.

- `runOn` - a [stormname:core.lang.RunOn] object that describes in which context the function that
  is being compiled is expected to run. This information is used by function calls to determine when
  it is necessary to send a message to another thread. We can ignore this member in this example.

In our class, we can define the function like this:

```bs
class RepeatStmt extends Expr {
    // ...
    void code(CodeGen gen, CodeResult result) : override {
        // ...
    }
    // ...
}
```

Now that we have some idea of the situation, we can start thinking about what code we want to
generate. A general strategy when generating intermediate code for higher-level statements is to
think of other parts of the code as placeholders and create a small skeleton that surrounds the
placeholders in a sensible way. In this particular case, we have the expression that determines how
many times the loop should be executed and the body of the loop. We call them `<times>` and `<body>`
respectively. Using them, we can implement the repeat syntax like below:

```
    # ...

    # store the result of <times> in <counter>
loopStart:
    cmp <counter>, 0
    jmp loopEnd, ifEqual

    # execute <body>

    sub <counter>, 1
    jmp loopStart
loopEnd:
    # ...
```

In the code above,`<counter>` is a temporary variable we need to create, and `loopStart` and
`loopEnd` are two labels. With this information, we "only" need to implement code that generates the
code we wrote above. We can do this line-by-line, and examine the result after each step by simply
printing the `CodeGen` variable. Note that this will also show any code generated by statements
*before* our `repeat` block. So if we simply add `print(gen.toS);` to the empty `code` function, we
get output similar to this, even if we have not yet produced any new code:

```
Running on: any
Current block: Block1
Block 0 {
    Block 1 {
    }
}
Dirty registers: 
                     | prolog
                     | location /home/filip/Projects/storm/extension/test.bs(38-70)
                     | begin Block1
```

The firsst line, "Running on:" prints the current value of the `runOn` member. It shows `any` since
the function `main` may run on any thread in the system. The second line, "Current block:", reflects
the value of `block`. It is currently `Block1` to indicate that new variables should be added to
block 1.

The next few lines illustrate all blocks (and all variables) currently emitted. At the moment, there
are two blocks: block 0 (the root block), that contains another block (block 1). No variables are
created yet. After the block table is the line "Dirty registers:". It will display all registers
that are used in the listing, but since we do not yet use any registers, it is empty. When
generating code, it is usually safe to ignore this line, as it is mostly useful when creating
backends, or when modifying the code.

Finally, there are three lines that starts with pipes (`|`). These lines each represent a single
instruction in the intermediate language. The space to the left of the pipe character will be used
to show all registers that contain useful data. Again, this information is mostly useful when
analyzing the code, and can usually be ignored when simply generating code.

The first instruction is the `prolog` instruction that sets up the call stack and performs other
housekeeping tasks. The second is a `location` instruction that simply contains metadata about the
code being executed. The third, `begin`, indicates that block 1 should start, and that all variables
in that block should be accessible.

Now that we know what we have to work with, we can start producing new code. The first thing we need
to do is to create a variable `counter`. We can do this by calling the `createVar` function as
follows.

```bsstmt:use=lang.bs.macro
VarInfo counter = gen.createVar(named{Nat});
```

This function simply creates a variable in the current block and gives us information about it
inside the `VarInfo` structure. Most importantly, the `VarInfo` contains the member `v` that
contains the [stormname:core.asm.Var] that is used in the intermediate language. The `VarInfo` type
stores some additional metadata that is required for initialization of complex types to work
correctly. We can, however, ignore that for now.

Now that we have created a `counter` variable, we can ask `times` to generate code that stores the
result in our variable. As with the `code` function we are currently implementing, the `code`
function of `times` requires two parameters: a `CodeGen` instance, and a `CodeResult` instance. For
the `CodeGen` instance, we can simply forward the one we received, as we want `times` to place its
generated code there. We do, however, need to create a new `CodeResult` for the second parameter to
instruct `times` to place the result in the variable we just created. Luckily there is a constructor
that does exactly that:

```bsstmt:use=lang.bs.macro
times.code(gen, CodeResult(named{Nat}, counter));
```
If we print the `CodeGen` after the added lines, we will now see an output similar to this:

```
Running on: any
Current block: Block1
Block 0 {
    Block 1 {
          0: size 04(04) free never using <none>
    }
}
Dirty registers: 
                     | prolog
                     | location /home/storm/extension/test.bs(38-70)
                     | begin Block1
                     | mov i[Var0], i5
```

We can see that our created variable is present inside block 1 (the line "0: size 04(04)..."), and
that a new `mov` instruction has been added to the listing that loads the value 5 into our variable,
Var0.

The next step is to emit the label `loopStart`. Since it does not exist yet, we must first create it
by calling `gen.l.label()`. We can then append it to the listing using the `<<` operator:

```bsstmt
Label loopStart = gen.l.label();
gen.l << loopStart;
```

After adding the label, we can add the `cmp` and the `jmp` instructions in the same way. The `jmp`
instruction needs to refer to the label `loopEnd`, so we have to create that as well. Note that we
do not add it to the listing yet, since we want it to point to a later position in the code.

```bsstmt
Label loopEnd = gen.l.label();
gen.l << cmp(counter.v, natConst(0));
gen.l << jmp(loopEnd, CondFlag:ifEqual);
```

The `natConst` function is used to "convert" a regular number into an `Operand` that can be used in
the intermediate language. This conversion is explicit since the intermediate language needs to know
the size of the literal values. Here we state that explicitly since we call `natConst` as opposed to
`wordConst` or `ptrConst`.

If we run the program now, we can verify that we emitted the code we wanted:

```
Running on: any
Current block: Block1
Block 0 {
    Block 1 {
          0: size 04(04) free never using <none>
    }
}
Dirty registers: 
                     | prolog
                     | location /home/storm/extension/test.bs(38-70)
                     | begin Block1
                     | mov i[Var0], i5
1: 
                     | cmp i[Var0], i0
                     | jmp pLabel2, equal
```

Here, we can see that the label `loopStart` got the index 1 in the listing, and `loopEnd` got the
index 2. However, we have not yet defined where label 2 should point to, so if we pass 0 to the
repeat statement now, it will likely crash the system.

The next step is to emit the body of the loop. Again, we can do this by calling the `code` member of
`body`. Since we do not worry about the result of `body`, we do not have to create any variables to
store the result in. We can simply tell it that we are not interested in the variable by passing a
default-constructed `CodeResult` object:

```bsstmt
body.code(gen, CodeResult());
```

If we run the program at this point, we will see a single `Hello` being printed, which is promising.
If we print the `CodeGen` object again, we can see that the print statement generated quite a bit of
code. This is because the `print` function is bound to the `Compiler` thread, and Storm must
therefore generate code to send a message to the `Compiler` thread. If you mark the `main` function
to run on the `Compiler` thread, you will see that much less code is generated.

Now, we only need to generate the end of the loop that decrements the counter and jumps back to the
start of the loop:

```bsstmt
gen.l << sub(counter.v, natConst(1));
gen.l << jmp(loopStart);
gen.l << loopEnd;
```

With this, we have emitted all code required for our repeat syntax. You can verify the code by
printing it and running it. Note, however, that the `loopEnd` variable will not be visible until
another instruction is added to the listing. If you wish to verify, you can add a `nop` instruction
(`gen.l << nop`).


Map Syntax Using the Basic Storm AST
------------------------------------

