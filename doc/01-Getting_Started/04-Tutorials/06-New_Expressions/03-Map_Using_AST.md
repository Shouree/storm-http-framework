Map Syntax Using the Basic Storm AST
====================================

This page assumes that you have followed the [setup](md:..) for the syntax extension guide already,
so that you have somewhere to work. In this stage we will implement the `map` syntax in terms of
Basic Storm. Compared to the `repeat` syntax, the `map` syntax has the added complexity that it
introduces new variables that should only be visible inside the body of the construct. As such, we
need to also consider the scoping of our new construct.

Syntax
------

The first step is, as with the [`repeat` statement](md:Repeat_Using_AST), to create the grammar for
the new syntax. We do this by creating a file `syntax.bnf` inside the `map` directory we created in
the [setup](md:..). As previously, we start by defining the the delimiters:

```bnf
use lang.bs;

optional delimiter = SDelimiter;
required delimiter = SRequiredDelimiter;
```

After this, we add a production that defines the grammar for our extension. Since the `map` syntax
acts as an expression, we extend `SExpr` instead of `SStmt` this time. As previously, we start by
adding the production itself and worry about the transforms later. We can write the map syntax as
follows:

```bnf
SExpr : "map", "(", SExpr ~ "for" ~ SName ~ "in" ~ SExpr, ")";
```

Note that we use the `~` separator instead of `,` around the keywords `for` and `in`. Since `SExpr`
and `SName` may be plain identifiers, it is useful to require a whitespace around them to avoid
surprises. For example, if the `~` after `for` and `in` were replaced with a `,` it would be
possible to write: `map(x forx inarray)`. This does not look sensible, and would likely surprise
developers, so we disallow it.

As a sidenote: this ambiguity does not introduce problems in this particular case. It is, however,
useful to be careful when working with grammars, as they may inadvertently interfere with other
parts of the grammar. For example, if the `spawn` keyword in Basic Storm would not require a space
after it, then the expression: `spawnThread()` would parse as `spawn Thread()`, which is most likely
*not* wat the developer intended.

We also add annotations for syntax highlighting of the keywords:

```bnf
SExpr : "map" #keyword, "(", SExpr ~ "for" #keyword ~ SName #varName ~ "in" #keyword ~ SExpr, ")";
```

We are now ready to test that our grammar behaves as we expect. In the `test.bs` file we used
before, add the following code to use our new syntax:

```bsstmt:use=tutorials.expressions.ast
Int[] values = [8, 10, 3];
Int[] result = map(x + 2 for x in values);
print(result.toS);
```

As in the previous example, we also need to include the package where the syntax is located. To do
this, we need to add the line `use expressions:map` to the top of the file. After doing this, we can
run the test program (by typing `storm .` in the terminal) to see if the grammar works as expected.
Since we have not yet implemented the semantics, we expect to get some form of error at this point,
as long as it is not a parse error. And indeed, running the example produces the following error
message:

```
@/home/storm/expressions/map/syntax.bnf(89-177): Syntax error:
No return value specified for a production that does not return 'void'.
```

This makes sense since we have not specified a return value for our production yet.


Semantics
---------

Since the syntax seems to be working as expected, we can now start working on the semantics of the
`map` expression. To simplify the implementation a bit, we assume that we are working with arrays of
integers. It is of course possible to lift this limitation if we wished to, but it requires some
care to get type inference right with regards to array literals. Working only with integers also
simplifies the implementation of the IR version of the implementation.

Since the `for` syntax needs to provide a loop variable that is only visible to the expression
"inside" the construct, we need to create a new scope for this expression. In Basic Storm, we do
this by creating a `Block` instance. Each `Block` instance roughly corresponds to a pair of curly
braces (`{}`) in that it introduces a new scope where we can store local variables.

The `Block` class actually corresponds to something that is slightly less than a pair of curly
braces (`{}`). Namely, a `Block` only provides a place to store variables. It does not contain any
expressions. Luckily, Basic Storm provides a subclass that both provides a new scope and is able to
contain expressions. This class is called `ExprBlock` since it contains a list of expressions. As
such, we will use the `ExprBlock` here to simplify our implementation. The IR implementation of the
`map` construct will use a plain `Block` for this purpose since we do not need the ability to store
expressions there.

As such, we can start our implementation by creating a new file called `semantics.bs` and adding the
following code to it:

```bs
use lang:bs;
use lang:bs:macro;
use core:lang;
use core:asm;

class MapExpr extends ExprBlock {
    init(SrcPos pos, Block parent) {
        init(pos, parent) {}
    }
}
```

Note that the constructor of `ExprBlock` (and also `Block`) requires two parameters: both a `SrcPos`
(the same as `Expr`) and a `Block`. The `Block` parameter specifies the block inside which the new
block is created. This makes the scope in Basic Storm hierarchichal, meaning that the system does
not only look inside the currently active block for variables, but also in enclosing blocks. In the
case of our `map` syntax, this makes it possible to use not only the loop variable in the
expression, but also variables from outside the loop.

We can then start to add annotations to the grammar to create our new class. As a start, we just
create the `MapBlock` as follows:

```bnf
SExpr => MapBlock(pos, block)
  : "map" #keyword, "(", SExpr ~ "for" #keyword ~ SName #varName
  ~ "in" #keyword ~ SExpr, ")";
```

If we run the program now, we get a different error. This time the error points to the code in our
test program:

```
@/home/storm/expressions/test.bs(130-136): Syntax error:
No appropriate constructor for core.Array(core.Int) found. Can not initialize
'result'. Expected signature: __init(core.Array(core.Int), void)
```

This error occurs because we have essentially transformed the `map` expression into an empty block.
As such, our test code is now equivalent to:

```bsstmt:use=tutorials.expressions.ast
Int[] values = [8, 10, 3];
Int[] result = {};
print(result.toS);
```

Since an empty block evaluates to `void`, it is clear why the error arises when the compiler
attempts to initialize `result`. This just means that we need to fill our block with some useful
code!

As a starting point, we can create some simple code using the `pattern` macro again and add that to
our block by calling the `add` function:

```bsclass:use=lang.bs.macro
init(SrcPos pos, Block parent) {
    init(pos, parent) {}

    add(pattern(this) {
            Array<Int> out;
            out << 9;
            out;
        });
}
```

For now, we create a block that creates an array (`out`), adds a single number to the array, and
then returns it back to the caller. We then add this block to our `MapExpr` by calling the `add`
function. If we run the code at this point, it will produce an array that only contains the number 9
without any errors.

To inspect the generated AST, we can simply print the `MapExpr` block at the end of the constructor
by adding a `print` statement: `print(this.toS)`. Currently, it produces the following output:

```
{
    {
        core.Array(core.Int) out(core.Array(core.Int)());
        out << 9;
        out;
    };
}
```

As we can see, we do indeed have two nested blocks. The outermost one is the `MapExpr` block, and
the innermost one is the block inside the `pattern` that we added. We do not technically need two
blocks like this, but it simplifies the implementation a bit and helps us hide internal variables.


Using the Input
---------------

The next step is to actually read data from the input array. We can do this similarly to the
`repeat` statement. The first step is to capture the expression in the grammar and pass it as a
variable to the `MapExpr` class. In the grammar, we simply bind the expression that corresponds to
the source array to the variable `src` and pass it to `MapExpr`:

```bnf
SExpr => MapExpr(pos, block, src)
  : "map" #keyword, "(", SExpr ~ "for" #keyword ~ SName #varName
  ~ "in" #keyword ~ SExpr(block) src, ")";
```

Then, we modify the `MapExpr` constructor to accept the new parameter and use it in our pattern:

```bsclass:use=lang.bs.macro
init(SrcPos pos, Block parent, Expr src) {
    init(pos, parent) {}

    add(pattern(this) {
            Array<Int> original = ${src};
            Array<Int> out;
            for (Nat i = 0; i < original.count(); i++) {
                out << original[i];
            }
            out;
        });
}
```

Note that we start by saving the value of the expression `${src}` in the variable `original` in the
beginning of the pattern, rather than using `${src}` throughout. Since `${src}` *inserts* the
expression `src` into all locations where it appear, using `${src}` more than once would cause the
expression in `src` to be evaluated more than once. Since we simply supply a variable in our
example, this would be fine. However, if `src` were a more complex expression that potentially had
side-effects (like function calls), this would cause our `map` expression to not behave according to
the programmer's expectations. Assigning `${src}` to a variable with the proper type has the
additional benefit that Basic Storm will automatically perform any type conversions for us during
the assignment, so we do not need the call to `expectCastTo` call that was required in the `repeat`
syntax. We can still insert it if we wish to, as it does not do any damage.

The remainder of the pattern simply copies the contents of `original` to `out` using a loop. There
are, of course, other ways to do this more concisely, but we will use this structure to actually
perform transform each element later on.

If we run the program now, we will find that the program (unsurprisingly) produces a copy of our
original array. Again, it is possible to print the `MapExpr` at the end of the constructor to see
what the pattern actually expanded to:

```
{
    {
        core.Array(core.Int) original = values;
        core.Array(core.Int) out(core.Array(core.Int)());
        {
            core.Nat i = 0;
            for (i < core.ArrayBase.count(original); core.Nat.*++(i)) {
                out << core.Array(core.Int).[](original, i);
            };
        };
        out;
    };
}
```

In particular, we can see that `${src}` was replaced by the variable `values`, since we used that
variable in our test program. We can also see that the `for` loop actually expands to two blocks.
One that declares the loop variable, and one that implements the remainder of the loop logic.


Transforming Values
-------------------

The final step of our implementation is to use the second expression to actually transform the
values. To do this, we need to update the grammar to bind the relevant parts of the match to
variables and pass them to our `MapExpr` class:

```bnf
SExpr => MapExpr(pos, block, src, var, tfm)
  : "map" #keyword, "(", SExpr(block) tfm ~ "for" #keyword ~ SName var #varName
  ~ "in" #keyword ~ SExpr(block) src, ")";
```

Then we modify the constructor of `MapExpr` so that it creates a variable with the specified name
and adds it to itself:

```bsclass:use=lang.bs.macro
init(SrcPos pos, Block parent, Expr src, SStr varName, Expr tfm) {
    init(pos, parent) {}

    Var local(this, named{Int}, varName, Actuals());
    add(local);

    // The remainder of the constructor is as before.
}
```

The class `Var` is a variable declaration statement. The constructor call corresponds to a
declaration like `Int x();`. The first parameter is the block in which the variable shall be
created, the second parameter is the type of the variable, the third is the name (and position, as a
`SStr`), and the final parameter is the parameters to the constructor call. We wish to call the
default constructor, so we pass a default-constructed `Actuals` object.

Creating the variable declaration in this manner will automatically create a `LocalVar` object that
describes the actual variable and add it to the block passed as the first parameter. To properly
initialize the variable we do, however, also need to add the variable declaration statement to the
block. We do this using the `add` function as before. At this point, we can run our program and
verify that it works as intended. If we print the `MapExpr` class, we can see that the
implementation currently produces the following code:

```
{
    core.Int x;
    {
        core.Array(core.Int) original = values;
        core.Array(core.Int) out(core.Array(core.Int)());
        {
            core.Nat i = 0;
            for (i < core.ArrayBase.count(original); core.Nat.*++(i)) {
                out << core.Array(core.Int).[](original, i);
            };
        };
        out;
    };
}
```

That is, we declare the variable `x` in the outermost scope, just as we hoped!

So far, all seems well. Let's proceed to actually use the variable with the expression. We wish to
produce a loop that looks like this:

```bsstmt:placeholders
for (Nat i = 0; i < original.count(); i++) {
    <varName> = original[i];
    out << <tfm>;
}
```

That is, we first set the variable we created to the element that we found at the current index of
the array. After that, we can evaluate the `tfm` expression (which might use the variable) to
produce the transformed value for our array.

The first part, the assignment, requires us to be able to access the variable. Simply adding
`${varName} = original[i]` will sadly not work, since the `${}` syntax expects the expression that
appears inside of it to evaluate to an `Expr`. Luckily, Basic Storm provides an expression that lets
us access a variable. It is called `LocalVarAccess`, and we can simply give it a position and the
variable we wish to access. For the second part, we can simply insert `tfm` using `${tfm}` since
`tfm` is already an `Expr`. This means that we change our `pattern` to the following:

```bsstmt:use=lang.bs.macro
add(pattern(this) {
        Array<Int> original = ${src};
        Array<Int> out;
        for (Nat i = 0; i < original.count(); i++) {
            ${LocalVarAccess(pos, local.var)} = original[i];
            out << ${tfm};
        }
        out;
    });
```

If we run the program at this point, we will get a surprising error:

```
@/home/storm/expressions/test.bs(143-144): Syntax error:
Can not find "x" with parameters ().
```

In an attempt to debug the issue, we can try to print the `MapExpr` at the end of the constructor
once more. If we do this, we get the following output:

```
{
    core.Int x;
    {
        core.Array(core.Int) original = values;
        core.Array(core.Int) out(core.Array(core.Int)());
        {
            core.Nat i = 0;
            for (i < core.ArrayBase.count(original); core.Nat.*++(i)) {
                x = core.Array(core.Int).[](original, i);
                out << <x[invalid] + 2>;
            };
        };
        out;
    };
}
```

The output looks as we would expect, except for the last line inside the loop (`out << <x[invalid] +
2>`). The angle brackets (`<>`) are normal, they are simply used to clarify the precedence of
operators in the textual representation. However, we would expect it to read `out << <x + w>`, not
`out << <x[invalid] + 2>`. Why is `x` invalid in this location, when we could assign to `x` on the
line before?


Fixing the Scope
----------------

To understand why the error occurs, we must understand how Basic Storm resolves names.

In general, name resolution in Basic Storm happens as soon as nodes in the AST are created. This is
why it is necessary to pass a `Block` to the `SExpr` and `SStmt` rules in the Basic Storm syntax.
Otherwise the system would not know how to resolve names!

As a sidenote, this might make you wonder why the error was not reported as soon as it was
discovered. The reason for this is that some parts of Basic Storm need to be able to detect and
re-try failed name lookups. One example of this is the `=` operator. For example, consider an
assignment like `a.b = c`. However, `a.b` does not exist, so the `=` operator should attempt to
lookup `a.b(c)` instead to see if `b` is an assignment function in this context. To achieve this,
the `=` operator must be able to detect and recover from name resolution errors. With the current
implementation, it can simply examine the AST node that corresponds to the left-hand side and see if
it corresponds to an invalid name, and act accordingly. This would be more difficult if Basic Storm
reported name resolution errors directly.

The upside of this behavior is that it makes it clear why the local variables we introduced inside
the pattern are not visible to the `tfm` expression. Since we never use the block produced by the
`pattern` as a parent block for anything, the variables in the `pattern` block will never be
reachable by name lookups in the `tfm` expression, even after we make the variable in the `MapExpr`
itself visible.

Regardless, we need to ensure that the names that appear inside the `tfm` expression are resolved in
the proper scope. We can solve this in the grammar by passing the newly created `MapExpr` rather
than the parent block to the `tfm` expression. Luckily, the `MapExpr` is bound to the `me` variable in the grammar, so we can just replace `block` with `me` as follows:

```bnf
SExpr => MapExpr(pos, block, src, var, tfm)
  : "map" #keyword, "(", SExpr(me) tfm ~ "for" #keyword ~ SName var #varName
  ~ "in" #keyword ~ SExpr(block) src, ")";
```

Nowever, this introduces another issue. If we try to run the program now, we get the following
error:

```
@/home/storm/expressions/map/syntax.bnf(89-247): Syntax error:
Can not use 'me' this early!
```

If we examine the grammar a bit more closely, we can see the problem. We have specified that in
order to create `tfm`, the Syntax Language needs to create the contents of the variable `me`.
However, to create the variable `me` requires calling `MapExpr(pos, block, src, var, tfm)`, which
involves having created `tfm`. As such, `tfm` must be created before `me`, and `me` must be created
before `tfm`. That is not possible.

To solve the issue, we must allow the `MapExpr` to be created without the `tfm` expression. We can
do this by creating another `ExprBlock` in the `MapExpr` constructor, and then later insert the
`tfm` expression into that block. To do this, we modify the constructor to not require the `tfm`
parameter:

```bs
use lang:bs;
use lang:bs:macro;
use core:lang;
use core:asm;

class MapExpr extends ExprBlock {
    private ExprBlock loopBody;

    init(SrcPos pos, Block parent, Expr src, SStr varName) {
        init(pos, parent) {
            loopBody(pos, parent);
        }

        Var local(this, named{Int}, varName, Actuals());
        add(local);

        add(pattern(this) {
                Array<Int> original = ${src};
                Array<Int> out;
                for (Nat i = 0; i < original.count(); i++) {
                    ${LocalVarAccess(pos, local.var)} = original[i];
                    out << ${loopBody};
                }
                out;
            });
    }
}
```

This change essentially replaces the usage of `tfm` with another `ExprBlock` called `loopBody`. We
use the loop body as a placeholder to make it easy to insert the actual loop body later on.

Since the constructor of `MapExpr` no longer requres the `tfm` expression as a parameter, we can
resolve the issue in the grammar by simply removing the last parameter to `MapExpr` (to make it read
`MapExpr(pos, block, src, var)`). If we run the program now, it will produce another error:

```
@/home/storm/expressions/map/semantics.bs(491-493): Syntax error:
Can not find an implementation of the operator << for core.Array(core.Int)&, void.
```

To understand why the error occurred, we can once again print the generated syntax tree:

```
{
    core.Int x;
    {
        core.Array(core.Int) original = values;
        core.Array(core.Int) out(core.Array(core.Int)());
        {
            core.Nat i = 0;
            for (i < core.ArrayBase.count(original); core.Nat.*++(i)) {
                x = core.Array(core.Int).[](original, i);
                out << {
                };
            };
        };
        out;
    };
}
```

From this output, it is fairly easy to see the issue. The last line in the loop currently reads:
`out << {};`. Again, since empty blocks evaluate to `void`, we are currently trying to insert a
value of the type `void` into an array that stores integers. This is what the compiler complains
about.

Luckily, it is fairly easy to fix the error. We simply need to insert the `tfm` expression into our
empty block. We can do this conveniently by creating a member function in the `MapExpr` class that
we call from the grammar. We call the function `setTransform` and implement it as follows:

```bs
void setTransform(Expr expr) {
    loopBody.add(expr);
}
```

We can then call the function from the grammar by replacing `tfm` with `-> setTransform`. This means
that the syntax transform will call `me.setTransform(<node>)` when the variable `me` is created,
which is exactly what we need. As such, we can change our production into the following:

```bnf
SExpr => MapExpr(pos, block, src, var)
  : "map" #keyword, "(", SExpr(me) -> setTransform ~ "for" #keyword ~ SName var #varName
  ~ "in" #keyword ~ SExpr(block) src, ")";
```

And with that, the circular dependency is resolved while making sure that the transform expression
is passed to the `MapExpr` in the end. To inspect the final tree, we can add a `print` statement to
the end of the `setTransform` function. It produces the following output, which is what the compiler
eventually turns into machine code:

```
{
    core.Int x;
    {
        core.Array(core.Int) original = values;
        core.Array(core.Int) out(core.Array(core.Int)());
        {
            core.Nat i = 0;
            for (i < core.ArrayBase.count(original); core.Nat.*++(i)) {
                x = core.Array(core.Int).[](original, i);
                out << {
                    x + 2;
                };
            };
        };
        out;
    };
}
```

As we can see, the expression `x + 2` was inserted into the previously empty block, which caused the
block evaluate to the correct type. Note that since we embedded the transform expression inside a
block, it is necessary to use a call to `expectCastTo` to make sure that automatic type inference
works properly in some cases. For simplicity, this was omitted from the code above.

For completeness, the final implementation of the `MapExpr` class is as follows:

```bs
use lang:bs;
use lang:bs:macro;
use core:lang;
use core:asm;

class MapExpr extends ExprBlock {
    private ExprBlock loopBody;

    init(SrcPos pos, Block parent, Expr src, SStr varName) {
        init(pos, parent) {
            loopBody(pos, parent);
        }

        Var local(this, named{Int}, varName, Actuals());
        add(local);

        add(pattern(this) {
                Array<Int> original = ${src};
                Array<Int> out;
                for (Nat i = 0; i < original.count(); i++) {
                    ${LocalVarAccess(pos, local.var)} = original[i];
                    out << ${loopBody};
                }
                out;
            });
    }

    void setTransform(Expr expr) {
        loopBody.add(expectCastTo(expr, named{Int}, scope));
    }
}
```
