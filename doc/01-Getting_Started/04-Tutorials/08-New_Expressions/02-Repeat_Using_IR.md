Repeat Syntax Using the Intermediate Language
=============================================

This page assumes that you have followed the [setup](md:..) for the syntax extension guide already,
so that you have somewhere to work. This page describes how to re-implement the [repeat
syntax](md:Repeat_Using_AST) using the Intermediate Language. Since the grammar is identical, we
will not modify it here. If you have not already implemented the `repeat` grammar, copy it from the
[previous part](md:Repeat_Using_AST). Also make sure you have added the example code to the
`test.bs` file as well, as we will use it for testing purposes throughout this page.

The main reason why we might wish to generate code in the Intermediate Language directly, rather
than relying on Basic Storm, is to give Basic Storm entirely new capabilities with constructs that
are not expressible in Basic Storm already. Apart from that, some type of operations are actually
simpler or more efficient to implement in the Intermediate Language directly, rather than relying on
Basic Storm.

To generate code in the Intermediate Language, we need to create a new AST node that we can embed
into the Basic Storm AST. We can do this by simply creating a new class that inherits from
[stormname:lang.bs.Expr]. Since we are implementing a repeat statement, we call it `RepeatStmt`. You
can either replace the code in `semantics.bs` with this code, or create a new file to store it in.

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


