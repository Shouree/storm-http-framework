Repeat Syntax Using the Basic Storm AST
=======================================

This page assumes that you have followed the [setup](md:..) for the syntax extension guide already,
so that you have somewhere to work. In this first stage, we will implement the `repeat` syntax as a
form of macro using the facilities already present in Basic Storm.

Syntax
------

The first step is to create the grammar for the new syntax. To do this, create a file `syntax.bnf`
inside the directory `repeat`. Inside the file we start by using the `lang.bs` namespace to easily
access the productions in Basic Storm, and then we define the delimiters as in the
[previous](md:../Using_Grammar) tutorial:

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


Semantics
---------

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

