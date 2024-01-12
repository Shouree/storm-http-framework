Map Syntax Using the Intermediate Language
==========================================

This page assumes that you have followed the [setup](md:..) for the syntax extension guide already,
so that you have somewhere to work. This page describes how to re-implement the
[map](md:Map_Using_AST) syntax using the Intermediate Language. Since the grammar is identical, we
will not modify it here. So if you have not already implemented the `map` grammar, copy it from the
end of the [previous part](md:Map_Using_AST). Also make sure you have added the example code to the
`test.bs` file as well, as we will use it for testing purposes throughout this page.

Creating an AST Node
--------------------

Similarly to the [`repeat` syntax](md:Repeat_Using_IR), we need to create a node that can be
inserted into the Basic Storm AST in order to be able to generate intermediate code at the right
location. Since the `map` syntax introduces additional variables, we need to inherit the `Block`
class rather than from the `Expr` class. Since we will be generating code directly, there is no
benefit from inheriting from the `ExprBlock` class as we did when we used the Basic Storm AST.

As such, to get started we can replace the old `MapExpr` class with a minimal definition that
defines the operations that are used by the syntax:

```bs
use lang:bs;
use lang:bs:macro;
use core:lang;
use core:asm;

class MapExpr extends Block {
    init(SrcPos pos, Block parent, Expr src, SStr varName) {
        init(pos, parent) {}
    }

    void setTransform(Expr expr) {}
}
```

Since the map expression does not do anything at the moment, we get the following error if we try to
run the example (using `storm .`):

```
@/home/storm/expressions/test.bs(130-136): Syntax error:
No appropriate constructor for core.Array(core.Int) found. Can not initialize 'result'.
Expected signature: __init(core.Array(core.Int), void)
```

As we have seen previously, this message arises because our AST node currently reports that it
evaluates to `void`. Since we use attempt to assign the result of our AST node to a variable, the
system will report this error.

To fix this error, we need to tell the system that the `MapExpr` node evaluates to an expression of
the type `Array<Int>` (or `Int[]`). Like before, we do this by overriding the `result` member like
so:

```bsclass:use=lang.bs.macro
ExprResult result() : override {
    ExprResult(Value(named{Array<Int>}));
}
```

If we run the program now, we will not get a type error. However, we will instead get the message
that the abstract function `blockCode` was called. This function is similar to the `code` function
in the `Expr` class in that it is called to generate code for the contents of the block. Actually,
since the `Block` class inherits directly from the `Expr` class, the `Block` class overrides the
`code` function in `Expr` and generates code that initializes the block and then calls `blockCode`.
There is, however, no default implementation of the `blockCode` function, which is why we see the
error now. This is also the reason why we used `ExprBlock` in the previous page. The `ExprBlock`
does implement `blockCode` by simply generating code for all expressions that have been added to it.
Since `Block` does only contains variables, it is not possible for the `Block` class to provide a
default implementation.

Anyway, to fix the problem, we can simply add a definition of the `blockCode` function to our class:

```bsclass
void blockCode(CodeGen gen, CodeResult res) : override {}
```

This time, the program will compile and start running. However, it will crash just after the
`MapExpr` with a `Memory access error`. This is not surprising since the `MapExpr` node promised to
produce a `Array<Int>`, but did not actually do so.


Creating an Array
-----------------

In order to fix the crash, we need to generate code that creates and returns an array in the
`blockCode` function. It is a bit cumbersome to do this directly in the Intermediate Language, since
we need to take care of allocating memory and executing the constructor. Luckily, the [code
generation library](md:/Library_Reference/Compiler_Library/Code_Generation) in Storm provides the
function `allocObject` that helps us with this. As such, we can create a variable to store the
result as follows:

```bsstmt:use=lang.bs.macro
core:asm:Var resultVar = res.location(gen);
allocObject(gen, named{Array<Int>:__init<Array<Int>>}, [], resultVar);
res.created(gen);
```

The first line simply retrieves the variable where the result of the `MapExpr` expression should be
stored. The next line calls `allocObject` to generate the code for creating the array. The call
takes four parameters. The first one is the `CodeGen` object that should receive the generated code,
and that contains metadata about the current code generation status. The second parameter is a
`Function` that is expected to be the constructor to call. Here, we use the `named` syntax to find
the default constructor for the `Array<Int>` class (remember: the `this` parameter is explicit). The
next parameter is an array of parameters to the constructor. Since we call the default constructor,
we do not need to supply any parameters. The final parameter is the variable where the created
object should be stored.

Finally, the last row informs the `CodeResult` class that we have initialized the result, so that it
can set up exception handling if required. In this particular case, however, `created` is a no-op
since heap allocations are cleaned up anyway. It is, however, still good practice to call `created`
to future-proof the code.

If we run the program now, we see that it no longer crashes and prints an empty array. A good first
step!

As before, it is possible to view the generated code by printing the `gen` object at the end of the
`blockCode` function. By doing this we can see that the `allocObject` function generated the
following code in this case:

```
fnParam p@core.Array(core.Int) - pointer:04(04)/08(08)@+0x00
fnCall p[Var0], p@C++:allocType - pointer:04(04)/08(08)@+0x00
fnParam p[Var0] - pointer:04(04)/08(08)@+0x00
fnCall p@core.Array(core.Int).__init(core.Array(core.Int)) - none:00(01)@+0x00[member]
```

That is, it first calls the built-in function `allocType` to allocate memory for the object. It then
calls the constructor to initialize the memory.


Storing the Captured AST Nodes
------------------------------

In order to generate the correct code for the `map` expression, we must first make sure to actually
store the expressions we captured from the grammar. Otherwise, it will be hard to do something
useful in the `blockCode` function.

We need three things in order to generate relevant code: the expression that evaluates to the source
array (`src`), the name of the variable used in the expression `varName`, and the expression that
should be evaluated to transform individual elements (received in `setTransform`). The expression
`src` is easy, we can simply store it in the class and use it later. We also wish to store the
transform expression in a similar way. However, since we do not receive it in the constructor we
create a `Maybe<Expr>` (i.e. `Expr?`) variable so that we can leave it uninitialized in the
constructor and set it later:

```bs
use lang:bs;
use lang:bs:macro;
use core:lang;
use core:asm;

class MapExpr extends Block {
	private Expr src;
	private Expr? transform;

	init(SrcPos pos, Block parent, Expr src, SStr varName) {
		init(pos, parent) {
			src = expectCastTo(src, named{Array<Int>}, parent.scope);
		}
	}

	void setTransform(Expr expr) {
		transform = expectCastTo(expr, named{Int}, scope);
	}

    // ...
}
```

As before, we use the function `expectCastTo` to help us type-check the source expression, and to
perform any implicit conversions if necessary.

The variable `varName` needs a bit more thought. We need to actually create the variable in the
constructor. Otherwise the transform expression will not be able to find it, which will eventually
result in a name error (as we saw in the previous page). We also save the created variable in a
member of the `MapExpr` class so that we can easily find the variable in the `blockCode` function
later. This way we do not have to look up the variable by name again.

We can achieve this by defining a member variable `boundVar` that we initialize in the constructor
and then add to the block:

```bs
use lang:bs;
use lang:bs:macro;
use core:lang;
use core:asm;

class MapExpr extends Block {
	private Expr src;
	private Expr? transform;
    private LocalVar boundVar;

	init(SrcPos pos, Block parent, Expr src, SStr varName) {
		init(pos, parent) {
			src = expectCastTo(src, named{Array<Int>}, parent.scope);
            boundVar = LocalVar(varName.v, named{Int}, varName.pos, false);
		}

        add(boundVar);
	}

    // ...
}
```

The constructor to `LocalVar` takes four parameters as follows: the first is the name of the
variable as a string. Since we have a `SStr`, we extract the string by reading the member `v`. The
second parameter is the type of the variable. We wish to store integers, so we simply give the type
`Int`. The third parameter is the location of the variable definition, which we retrieve from the
`varName` variable through the member `pos`. The final parameter indicates if the variable is a
parameter or not. Since this is not a function parameter, we simply pass `false`. After creating the
variable, we add it to the `MapExpr`, so that the transform expression can find it.


Generating Code
---------------

Now that we have all information that we need, we can finally start generating code for the `map`
construct. As before, we start by sketching the code we wish to generate with placeholders for the
different expressions. In this case, we wish to generate the following code:

```
    # ...

    # create an array, <resultVar>, that will be our result
    # evaluate <src>, store it in <originalArray>
    # evaluate <originalArray>.count(), store it in <originalCount>

    mov <loopCounter>, 0
loopStart:
    cmp <loopCounter>, <originalCount>
    jmp loopEnd, ifAboveEqual

    # extract element <loopCounter> from <originalArray>, store in <boundVar>
    # evaluate <transform>, store result in <transformed>
    # call <resultVar>.push(<transformed>)

    add <loopCounter>, 1
    jmp loopStart
    
loopEnd:
    # ...
```

The general structure is similar to that of the `repeat` statement. The contents of the loop is,
however, a bit more involved since we need to interact with more parts of the system.

Based on the outline above, we can now start generating code, one step at a time. Note that we have
already implemented the first step (create an array and store it in `<resultVar>`) to avoid the
previous crash.

The next step is therefore to evaluate `src` and store it in a temporary variable. We can actually
do this quite simply by utilizing the fact that the `CodeResult` can create the variable for us:

```bsstmt:use=lang.bs.macro
CodeResult originalArrayRes(named{Array<Int>}, gen.block);
src.code(gen, originalArrayRes);
core:asm:Var originalArray = originalArrayRes.location(gen);
```

The first line creates a `CodeResult`. We instruct it that we wish to receive the result in a
variable that is visible inside the currently active block, and that we expect to receive the type
`Array<Int>`. This means that we will either reuse a variable that already exists elsewhere in the
system, or that the `CodeResult` will create a suitable variable for us. Potentially reusing a
variable is perfectly fine, since we will only ever read from the variable, and never modify it.

The second line then asks the `src` expression to generate code, and to place its results in the
location we described using the `CodeResult` object. Finally, we extract the variable from the
`originalArrayRes` so that it is more convenient to use later on.

The next step is to call `<originalArray>.count` to find the length of the array. We can call
functions conveniently by using the member `autoCall` in the `Function` class to generate the code
for us. The `autoCall` uses the `CodeResult` class just like the `code` function for expressions, so
the code looks very similar to what we have written so far:

```bsstmt:use=lang.bs.macro
Function countFn = named{Array<Int>:count<Array<Int>>};
CodeResult originalCountRes(named{Nat}, gen.block);
countFn.autoCall(gen, [originalArray], originalCountRes);
core:asm:Var originalCount = originalCountRes.location(gen);
```

The first line simply stores the function we wish to call in a variable to make the code more
readable. The second line creates another `CodeResult` instance. Again, we instruct it that we wish
to have the result stored inside a variable that is visible in the active block. This time the type
should be `Nat` to match the return type of the `count` function.

In the third line, we call `autoCall` to generate the code necessary for actually calling the
function. The first parameter is the `CodeGen` instance where the generated code should be placed,
the second parameter is an array of parameters to the function. Here, we pass a single parameter
that is used as the `this` parameter. The third and final parameter is our `CodeResult` that
indicates where we want the result to be located.

The fourth and last line simply extracts the created variable and stores it in a variable for easy
access later.

After extracting the count, we are almost ready to start the loop. First, we need the loop counter.
Since we initialize the loop counter ourselves, rather than evaluating an expression to get it, we
need to create the variable manually. We can do this using the `createVar` function inside
`CodeGen`. We then initialize the variable to zero by moving the value zero into it. Note that the
last step is not strictly necessary since the Intermediate Language guarantees that variables are
initialized to zero by default. The `mov` instruction is shown here to illustrate the syntax.

```bsstmt:use=lang.bs.macro
core:asm:Var loopCounter = gen.createVar(named{Nat}).v;
gen.l << mov(loopCounter, natConst(0));
```

After creating the variable, we can emit the header of the loop. We start by creating a label that
we call `loopStart` and output it, so that we can jump back to the header of the loop later:

```bsstmt
Label loopStart = gen.l.label();
gen.l << loopStart;
```

After emitting the label, we emit code for checking if the loop should be terminated. In our case,
we should compare the variable `loopCounter` to the variable `originalCount`. We can do this with a
`cmp` instruction:

```bsstmt
gen.l << cmp(loopCounter, originalCount);
```

This sets an internal register in the machine that we can use for conditional jumps. We wish to exit
the loop if `loopCounter` is greater than or equal to `originalCount`. Since both values are
unsigned integers (`Nat`s), we use the condition `ifAboveEqual` for this (`ifGreaterEqual` can be
used for signed numbers). To tell the `jmp` instruction where it should jump to, we need to create
the label `loopEnd` already here. We will then emit the label at the end of the loop.

```bsstmt
Label loopEnd = gen.l.label();
gen.l << jmp(loopEnd, CondFlag:ifAboveEqual);
```

Next, we emit the body of the loop. The first thing we need to do is to get the current element from
the array. We achieve this by calling the `[]` operator like before. One detail to note with the
`[]` operator of the `Array` class is that it returns a *reference* to the element rather than the
element itself. This is to allow modifying the element in the array by assigning to it. This does,
however, mean that we need to take this into consideration when generating machine code, since we
need to dereference the returned value to retrieve the actual value. Basic Storm handles this
transparently for us. We can check for this in the code by examining the member `result.ref` inside
the function. If it is true, the function returns a reference (i.e. a pointer) to the value rather
than the value itself.

In spite of this the different return type, the call looks similar to the ones we have written
previously:

```bsstmt:use=lang.bs.macro
Function accessFunction = named{Array<Int>:"[]"<Array<Int>, Nat>};
CodeResult elemPtr(accessFunction.result, gen.block);
accessFunction.autoCall(gen, [originalArray, loopCounter], elemPtr);
```

There are two differences that might be interesting to observe. First, since the function `[]` is
not an identifier according to Basic Storm, we need to enclose it in quotes to be able to write it
in the `named` syntax. This is a special case in the `named` syntax to make it possible to retrieve
special members like the `[]` function. It is not possible to enclose parts in a name in quotes
anywhere else in Basic Storm.

The second difference is that we pass `accessFunction.result` to the constructor for `CodeResult`,
rather than simply specifying `named{Int}`. This is to instruct the `CodeResult` that we want a
reference to an integer rather than an integer. The constructor accepts a `Value` instance as its
first parameter, but since the default constructor of `Value` creates a non-reference by default,
simply calling `CodeResult(named{Int}, ...)` would not have the desired effect. We could, however,
write `CodeResult(Value(named{Int}, true), ...)` or `CodeResult(Value(named{Int}).asRef(), ...)`
instead. However, just passing the result type of the function makes the intent clearer in this
case.

After we have acquired a pointer to the current element, we can read the actual value and copy it to
the `boundVar` variable. The Intermediate Language allows us to follow pointers by using the
`intRel` operand. However, this operand requires that the pointer is in a register beforehand. Since
we do not care about the values in any of the three general purpose registers (`a`, `b`, and `c`) at
the moment, we can simply use any one of them. As such, we simply pick register `a` for the task.
Since we are goint to store a pointer in the register, we use the name `ptrA` to indicate that we
are using the pointer-sized version of the register `a`:

```bsstmt
gen.l << mov(ptrA, elemPtr.location(gen));
gen.l << mov(boundVar.var.v, intRel(ptrA));
```

In the first line, we load the value returned from the function (stored in `elemPtr`) into the
register `ptrA`. We then load an integer from the address that `ptrA` contains by using the operand
`intRel` (there are versions for different sizes as well), and store it in the variable we created
in the constructor. The member `var` contains a `VarInfo` type, which in turn stores the actual
variable inside its `v` member.

Note that the code above utilizes the fact that we are only working with integer types in this
implementation. If the array could contain other types, we would have to replace the second `mov`
with a copy operation that is appropriate for the contained type. For numeric and pointer types, it
is enough to replace `intRel` with `xRel` and specify the size. For value types, however, we need to
call the copy constructor, and ensure that any destructors are called at the end of the loop (or in
case of an exception). Another option would be to make `boundVar` into a reference rather than a
copy of the value. This means that it is possible to assign to the variable in the `map` syntax, but
it would avoid copies of value types.

After setting `boundVar` to the current element, we can simply evaluate the `transform` expression.
Since the `transform` variable might contain `null`, we must explicitly ensure that it does not
contain `null` before using it. Perhaps the cleanest way to do this is to add an `unless` block at
the start of the function:

```bsstmt
unless (transform)
    throw InternalError("No transform set for this MapBlock!");
```

After that, at the end of the function, we can evaluate the expression since Basic Storm now knows
that `transform` does not contain `null` in the remainder of the function:

```bsstmt:use=lang.bs.macro
CodeResult transformed(named{Int}, gen.block);
transform.code(gen, transformed);
```

Now that we have computed the new value, we can insert it into the new array by calling the `push`
member of the array. Similarly to the `[]` operator, the `push` member expects a reference to the
element to be inserted rather than a value. As such, we need to get the address of the variable by
using the `lea` (Load Effective Address) instruction in the Intermediate Language before passing it
to the `autoCall` function. We could create a separate variable to store the address, but since we
only have one temporary value like this, we can simply use the register `a` (using the name `ptrA`
to indicate that it is a pointer):

```bsstmt:use=lang.bs.macro
gen.l << lea(ptrA, transformed.location(gen));
Function pushFn = named{Array<Int>:push<Array<Int>, Int>};
pushFn.autoCall(gen, [resultVar, ptrA], CodeResult());
```

Note that since we are not interested in the result from the `push` function, we can simply pass a
default-constructed `CodeResult` to `autoCall` to let it know that we do not need the result. This
is a useful thing to do in most situations, as different parts of the system can utilize the
knowledge that the result will be discarded to avoid performing some computations. For example,
passing a default-constructed `CodeResult` to the AST node for an integer literal generates no code
at all, since the node knows that the result will not be used.

Now that we are done with the loop body, we can generate the end of the loop. We need to increment
the loop counter to advance to the next element, and then we can jump back to the top of the loop:

```bsstmt
gen.l << add(loopCounter, natConst(1));
gen.l << jmp(loopStart);
```

Finally, we need to define the location of the end of the loop by appending the `loopEnd` label to
the listing. Otherwise, the `jmp` instruction at the start of the loop will not know where to jump:

```bsstmt
gen.l << loopEnd;
```

And with this, we have implemented our plan, and we are ready to test our implementation. If you
have done everything correctly, the code should now work as expected.

For reference, the final implementation of the semantics are provided below:

```bs
use lang:bs;
use lang:bs:macro;
use core:lang;
use core:asm;

class MapExpr extends Block {
	private Expr src;
	private Expr? transform;
	private LocalVar boundVar;

	init(SrcPos pos, Block parent, Expr src, SStr varName) {
		init(pos, parent) {
			src = expectCastTo(src, named{Array<Int>}, parent.scope);
            boundVar = LocalVar(varName.v, named{Int}, varName.pos, false);
		}

        add(boundVar);
	}

	void setTransform(Expr expr) {
		transform = expr;
	}

	ExprResult result() : override {
		ExprResult(Value(named{Array<Int>}));
	}

	void blockCode(CodeGen gen, CodeResult res) : override {
		unless (transform)
			throw InternalError("No transform set for a MapBlock!");

		core:asm:Var resultVar = res.location(gen);
		allocObject(gen, named{Array<Int>:__init<Array<Int>>}, [], resultVar);
		res.created(gen);

		CodeResult originalArrayRes(named{Array<Int>}, gen.block);
		src.code(gen, originalArrayRes);
		core:asm:Var originalArray = originalArrayRes.location(gen);

		Function countFn = named{Array<Int>:count<Array<Int>>};
		CodeResult originalCountRes(named{Nat}, gen.block);
		countFn.autoCall(gen, [originalArray], originalCountRes);
		core:asm:Var originalCount = originalCountRes.location(gen);

		core:asm:Var loopCounter = gen.createVar(named{Nat}).v;
		gen.l << mov(loopCounter, natConst(0));

		Label loopStart = gen.l.label();
		gen.l << loopStart;

		Label loopEnd = gen.l.label();
		gen.l << cmp(loopCounter, originalCount);
		gen.l << jmp(loopEnd, CondFlag:ifAboveEqual);

		Function accessFunction = named{Array<Int>:"[]"<Array<Int>, Nat>};
		CodeResult elemPtr(accessFunction.result, gen.block);
		accessFunction.autoCall(gen, [originalArray, loopCounter], elemPtr);

		gen.l << mov(ptrA, elemPtr.location(gen));
		gen.l << mov(boundVar.var.v, intRel(ptrA));

		CodeResult transformed(named{Int}, gen.block);
		transform.code(gen, transformed);

		gen.l << lea(ptrA, transformed.location(gen));
		Function pushFn = named{Array<Int>:push<Array<Int>, Int>};
		pushFn.autoCall(gen, [resultVar, ptrA], CodeResult());

		gen.l << add(loopCounter, natConst(1));
		gen.l << jmp(loopStart);

		gen.l << loopEnd;
	}
}
```