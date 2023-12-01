Using Grammar in Storm
=====================

This tutorial shows how to use [the Syntax Language](md:/Language_Reference/The_Syntax_Language)
together with Basic Storm to parse simple arithmetic expressions. This tutorial involves
context-free grammars and regular expressions. It assumes that you have some knowledge of these
concepts, as the tutorial focuses on how to use context-free grammars and regular expressions in
Storm.

As with the other tutorials, the code produced by this tutorial is available in
`root/tutorials/grammar`. You can run it by typing `tutorials:grammar:main` in the Basic Storm
interactive top-loop.

Setup
-----

First, we need somewhere to work. Create an empty directory somewhere on your system. The name does
not matter too much, but it is easier to avoid spaces and numbers in the name since it will be used
as a package name in Storm. For example, use the name `grammar`. Then, open a terminal and change
into the directory you just created. This makes it possible to run the code we will produce by
typing the following in the terminal:

```
storm .
```

Note that it will currently only start the top-loop since we have not yet defined a `main` function.


Grammar for Expressions
----------------------

In this tutorial, we will create a simple grammar for parsing simple arithmetic expressions. We
start by writing the grammar in the Syntax Language. Create a file with the extension `.bnf` (for
example, `expr.bnf`, but the name does not matter) and open it in your editor.

We start by specifying the meaning of the `,` and `~` separators in the file. We do this using the
`optional delimiter` and `required delimiter` directives respectively:

```bnf
optional delimiter = SOptDelimiter;
required delimiter = SReqDelimiter;
```

This tells the system that whenever `,` appears, it should use the rule `SOptDelimiter`, and
whenever `~` appears, it should use the rule `SReqDelimiter`. These rules can have any name. We
could even use the rules from Basic Storm if we wished by specifying `lang.bs.SDelimiter` and
`lang.bs.SRequiredDelimiter`.

Since we have told the system to use `SOptDelimiter` and `SReqDelimiter`, we also need to define
them. We start by defining the rules themselves. Since they are used as delimiters, we are not
interested in capturing anything in these rules. Therefore we declare them as accepting no
parameters, and returning `void`:

```bnf
void SOptDelimiter();
void SReqDelimiter();
```

We must also define productions for these rules. Currently, the rules exists but does not match any
strings (not even the empty string). This means that wnenever `,` or `~` are used in the grammar,
the parser will reject all strings, which is not what we want.

To avoid this, we define two productions as follows. Each production contains a single regular
expression. The first one matches zero or more whitespace, and the second matches one or more
whitespace:

```bnf
SOptDelimiter : "[ \n\r\t]*";
SReqDelimiter : "[ \n\r\t]+";
```

With the delimiters out of the way, we can start writing the actual grammar. Since we have defined
the optional delimiter, we can use `,` in any place where spaces *may* appear. This makes the
grammar more compact and easy to read, since we do not have to write `SOptDelimiter` all over the
grammar.

At this point it might be worth talking about naming. As you can see, all productions have names
that start with an `S`. This is in no way required by Storm or the Syntax Language. It is, however,
convenient to prefix the names of things in the Syntax Language with `S` for two reasons:

1. It makes it very clear when we are referring to things in the Syntax Language.

2. It avoids name collisions between types generated for the parse tree, and types for other
   representations. For example, Basic Storm has a type `Expr` that represents a node in the
   Abstract Syntax Tree (AST). The syntax also has a rule that corresponds to an expression. This
   rule is named `SExpr`, since the type generated for the parse tree would otherwise have the same
   name as the corresponding AST node.

In this example we do not have other representations, so we could skip the `S` prefix. However, we
use it here to be consistent with the naming in other parts of Storm.

With this in mind, we can now write the grammar for parsing simple expressions. The grammar supports
`+`, `-`, `*`, and `/`. We split the grammar into three "layers" so that the different operators
have the correct priority.

The first rule, `SExpr` represents an entire expression. We define it as follows:

```bnf
void SExpr();
SExpr : SExpr, "\+", SProduct;
SExpr : SExpr, "-", SProduct;
SExpr : SProduct;
```

The first line defines the rule itself. For now, we do not worry about the parameters and return
type since we are only interested in parsing at the moment. The second line defines a production for
the `SExpr` line. It states that an expression can be an expression, followed by a `+` sign,
followed by a `SProduct`. Each of these three parts may be separated by zero or more spaces since
they are delimited by commas (and due to our definition of the optional delimiter). Note that we
need to escape the `+` character in the regular expression, since `+` has a special meaning in
regular expressions.

The third line is similar to the second, except that it defines subtraction instead of addition.
Finally, the fourth and last line states that an expression may simple be a `SProduction`. This
allows us to parse expressions that contains no `+` or `-` operators at the topmost level.

Next up is the `SProduct` rule. It represents a part of an expression that contains operators that
have higher priority than `+` and `-`. In our case, this is `*` and `/`. It has the same structure
as the rules and productions for `SExpr`:

```bnf
void SProduct();
SProduct : SProduct, "\*", SAtom;
SProduct : SProduct, "/", SAtom;
SProduct : SAtom;
```

Finally, the rule `SAtom` represents parts of an expression that are atomic with regards to
operators. In our case, it matches numbers (the second line), and expressions inside of parentheses:

```bnf
void SAtom();
SAtom : "[0-9]+";
SAtom : "(", SExpr, ")";
```

With all of these parts, our grammar is complete. We can now focus on how the grammar is used from
Basic Storm. Therefore, we need to create a file with the extension `.bs` to store or Basic Storm
code. For example, call the file `expr.bs`, but the name does not matter.

In the file, we add the following content to test the grammar:

```bs
use core:lang; // for Parser
use core:io;   // for Url

void eval(Str expr) on Compiler {
    Parser<SExpr> parser;
    parser.parse(expr, Url());
    print(parser.infoTree().format());
}

void main() on Compiler {
    eval("1 + 2");
}
```

The code first imports the package `core:lang` so that we can access the type `Parser` conveniently.
We also import `core:io` to access `Url` conveniently.

Next, we define the function `eval` that we will develop so that it evaluates an expression. Since
the parser and the grammar are all actors that execute on the `Compiler` thread, we declare this
function to run on the `Compiler` thread as well to avoid expensive thread switches.

Inside the `eval` function, the first step is to create a parser instance. The name of the starting
production is specified as a parameter to the type (just as with `Array`). So, `Parser<SExpr>` means
that we create a parser that starts in the rule `SExpr`. The parser automatically imports all
productions in the package that `SExpr` resides in.

After creating a parser, we can parse the input string by calling the `parse` member. It accepts two
parameters: first the string to be parsed, and then the path of the file that the string originates
from. Since we did not read our string from a file, we simply pass an empty `Url`. This is not a
problem since the parser only uses the second parameter for indicating the position of errors.

When the `parse` member has finished it work, we can extract the parse tree from the parser. For
now, we use the member `infoTree` for this. To see the tree in a textual representation, we call
`format` on the tree and finally `print` it.

Finally, to run the code conveniently, we define a `main` function that simply calls `eval` with an
example expression to see if our program works correctly. This means that we can run the example
from the terminal by running: `storm .` (assuming we are in the directory with that contains our two
files). For the expression `1 + 2`, this produces output like below (the numbers after `@` may
differ depending on the order of the productions):

```
{
    production: @ 2 of SExpr
    1 -> {
        production: @ 4 of SExpr
        1 -> {
            production: @ 7 of SProduct
            1 -> {
                production: @ 8 of SAtom
                1 -> '1' (matches "-?[0-9]+")
            }
        }
    }
    1 -> {
        production: @ 0 of SOptDelimiter
        1 -> ' ' (matches "[ \n\r\t]*")
    } (delimiter)
    1 -> '+' (matches "\+")
    1 -> {
        production: @ 0 of SOptDelimiter
        1 -> ' ' (matches "[ \n\r\t]*")
    } (delimiter)
    1 -> {
        production: @ 7 of SProduct
        1 -> {
            production: @ 8 of SAtom
            1 -> '2' (matches "-?[0-9]+")
        }
    }
}
```

The output above shows exactly which productions the parser matched. The names that start with `@`
are the anonymous names of the productions in our syntax. If we give them a name (by adding `=
<name>` before the semicolon), we can see the names of the productions as well. It is, however, not
very easy to read since it contains too many details.

Let's try to improve the readability by asking the parser for a parse tree rather than an info tree.
We can do this by replacing `print(parser.infoTree().format())` with `print(parser.tree().toS())`.
This gives us the following output:

```
{ ./(0-5)
    grammar.SExpr -> grammar.@ 3
}
```

This time the parse tree is too small. It only contains one node that corresponds to the starting
production. The line `grammar.SExpr -> grammar.@ 3` means that we printed a node that corresponds to
the production `grammar.@ 3` that extens the rule `grammar.SExpr`.


Binding Tokens to Variables
---------------------------

The reason why only one node appeared is that the Syntax Language only saves subtrees that are bound
to variables. To bind subtrees to variables, we can simply add a name after the token we wish to
save. In our case, we can do like this:

```bnf
void SExpr();
SExpr : SProduct p;
SExpr : SExpr l, "\+", SProduct r;
SExpr : SExpr l, "-", SProduct r;

void SProduct();
SProduct : SAtom a;
SProduct : SProduct l, "\*", SAtom r;
SProduct : SProduct l, "/", SAtom r;

void SAtom();
SAtom : "-?[0-9]+" nr;
SAtom : "(", SExpr e, ")";
```

If we run the program once more (using `storm .`), we get a more reasonable output:

```
{ ./(0-5)
    grammar.SExpr -> grammar.@ 2
    l : { ./(0-1)
        grammar.SExpr -> grammar.@ 4
        p : { ./(0-1)
            grammar.SProduct -> grammar.@ 7
            a : { ./(0-1)
                grammar.SAtom -> grammar.@ 8
                nr : 1@./(0-1)
            }
        }
    }
    r : { ./(4-5)
        grammar.SProduct -> grammar.@ 7
        a : { ./(4-5)
            grammar.SAtom -> grammar.@ 8
            nr : 2@./(4-5)
        }
    }
}
```

Lines that start with `{` indicate the start of a node that corresponds to a production. Right after
the `{` is the location in the input that matched the node. In the case of the root note, the entire
string was matched, so `(0-5)` is printed. The part before (`./`) is the `Url` we passed to `parse`.
The empty `Url` is printed as `./` since it is a relative path to the current directory. The first
line in each node is the name of the rule and production that were matched (as we saw above). Any
remaining lines are the variables that are bound. In the topmost node, we can for example see that
the variables `l` and `r` are bound, and contains other nodes. Eventually, we will see variables
that are bound to things that are indicated as `1@./(0-1)` for example. This is the representation
of text that matches a regular expression. In the case of `1@./(0-1)`, it represent that the text
`1` was matched at the source location after the `@` (i.e. in `./`, characters `0-1`).

At this point it is a good idea to test some different expressions and see how the corresponding
parse trees look.

Disallow Partial Matches
------------------------

One thing you might note is that the parse succeeds even though the whole string is not correct. For
example, parsing `1 + a` succeeds, but the parser only matches the first `1`. The parser behaves
this way to allow parsing prefixes of strings conveniently. As long as the parser manages to find a
match, it treats the parse as a success (this is why `1 + a` works, but not `a + 1` for example). It
is, however, easy to check whether it succeeded in parsing the entire string or not by calling the
`hasError` member. This function returns true if the parser has found an error anywhere in the
string, and false otherwise. It can be used like below:

```bs
void eval(Str expr) on Compiler {
    Parser<SExpr> parser;
    parser.parse(expr, Url());
    if (parser.hasError())
        throw parser.error();
    print(parser.tree().toS());
}
```


Interacting with the Parse Tree
-------------------------------

Just parsing may be fun, but we would also like to *use* the results of the parse for something
useful. Since rules and productions appear as classes in Basic Storm, let's try to inspect it that
way.

To be able to access individual productions from Basic Storm, we must first give them a name. We do
this by appending a name to the end of the productions. As a start, we only do this for the
productions for `SExpr`. We name the first one `Add`, the second one `Subtract`, and the third one
`Product` as below:

```bnf
void SExpr();
SExpr : SExpr l, "\+", SProduct r = Add;
SExpr : SExpr l, "-", SProduct r = Sub;
SExpr : SProduct p = Product;
```

The only difference from before is that the productions have a name that is not anonymous. This is
reflected in the textual representation of the parse tree. If we run the code we had before, we will
now see the names appear in the textual representation of the tree. The first line in the root node
contains `grammar.SExpr -> grammar.Add` instead of `grammar.SExpr -> grammar.@ 2`, which is a bit
easier to understand.

```
{ ./(0-5)
    grammar.SExpr -> grammar.Add
    l : { ./(0-1)
        grammar.SExpr -> grammar.Product
        p : { ./(0-1)
            grammar.SProduct -> grammar.@ 4
            a : { ./(0-1)
                grammar.SAtom -> grammar.@ 5
                nr : 1@./(0-1)
            }
        }
    }
    r : { ./(4-5)
        grammar.SProduct -> grammar.@ 4
        a : { ./(4-5)
            grammar.SAtom -> grammar.@ 5
            nr : 2@./(4-5)
        }
    }
}
```

These names also means that we can inspect the nodes from Basic Storm. Since both rules and
expressions appear as types to Basic Storm (and most other languages as well), and that the
production-types inherit from the corresponding rule-type. This means that we can inspect the parse
tree simply by using a downcast.

We first store the result of `parser.tree()` in a variable. The result has the type `SExpr` (the
same as the parameter to the `Parser`) since that is the rule we started parsing at:

```bsstmt
SExpr tree = parser.tree();
```

The `SExpr` type is abstract since it corresponds to a rule. That means that it will contain one of
the subclasses to `SExpr`. In our case we have three subclasses, one for each of the productions
`Add`, `Subtract`, and `Product`. We can check which one it is using the `as` operator in an
`if`-statement:

```bsstmt
if (tree as Add) {
    // ...
}
```

Remember that `tree` will have the type `Add` inside the `if`-statement. This means that we can
access the subtrees that correspond to the bound tokens as member variables inside `tree`. For
example, we can print the left- and right-hand sides as follows:

```
print("Addition of:");
print(tree.l.toS());
print("and:");
print(tree.r.toS());
```

If we do the same thing for the remaining productions, we get the following implementation of
`eval`:


```bs
void eval(Str expr) on Compiler {
    Parser<SExpr> parser;
    parser.parse(expr, Url());
    SExpr tree = parser.tree();
    if (tree as Add) {
        print("Addition of:");
        print(tree.l.toS());
        print("and:");
        print(tree.r.toS());
    } else if (tree as Sub) {
        print("Subtraction of:");
        print(tree.l.toS());
        print("and:");
        print(tree.r.toS());
    } else if (tree as Product) {
        print("Just a product:");
        print(tree.p.toS());
    }
}
```

As we can see, this quickly gets tedious to maintain. We have to write code that is tightly coupled
with the grammar just to extract the meaning of the parse. This structure also makes it difficult to
extend the grammar, since adding a new production would require adding another branch to the long
if-statements somewhere in the system.

Using Syntax Transforms
-----------------------

Due to the problems we saw above, it is not common to use names and type-casts to inspect the
grammar in Storm. Instead, the Syntax Language provides a mechanism, called *syntax transforms*, to
specify the semantics of the grammar along with the grammar itself. This allows the Syntax Lanugage
to generate code that traverses the grammar and transforms it into a more convenient representation.
Since this representation is written alongside the grammar, it also has the benefit that it allows
easy extension from other places.

In this tutorial, we will not use the grammar to generate an AST, which is often the case for
languages in Storm. Rather, we will use the syntax transforms to actually *evaluate* the parsed
expressions directly.

Since we will not need the production names anymore, we remove them an replace them with syntax
transforms instead. We start by modifying the rule definitions. Since we will use the transforms to
evaluate expressions, we want to return a value from the transform functions. To simplify the
semantics, we limit ourselves to integers and thereby replace the return type with `Int`:

```bnf
Int SExpr();
Int SProduct();
Int SAtom();
```

We also need to modify the productions to actually return a value. This is done by adding an arrow
(`=>`) followed by a simple expression that should be evaluated to create the value. Note that these
simple expressions in the Syntax Language may only consist of a single function call, a single
constructor call, or the name of a variable. We start with the productions for `SExpr`. The first
one (for addition) needs to compute the sum of the left hand side and right hand side of the `+`.
Since we have declared that both `SExpr` and `SProduct` return an `Int`, both `l` and `r` are
integers, which means that we can simply call the `+` operator in Storm by adding `=> +(l, r)` as
follows:

```bnf
SExpr => +(l, r) : SExpr l, "\+", SProduct r;
```

This means that whenever the rule is matched, we will first evaluate the left-hand side and bind it
to `l`, then the right-hand side and bind it to `r`, and finally call `+(l, r)` (which is the same
as `l + r` in Basic Storm), bind it to `me`, and finally return it to the caller.

We do the same thing for `-`:

```bnf
SExpr => -(l, r) : SExpr l, "-", SProduct r;
```

For the last production in `SExpr`, we do not need to perform any computations. We simply need to
forward the value of the `SProduct` match. We can do that by simply specifying the variable we wish
to return after the arrow (`=>`) like so:

```bnf
SExpr => p : SProduct p;
```

The three productions for `SProduct` are nearly identical to the productions in `SExpr` and can be
modified in the same way:

```bnf
SProduct => *(l, r) : SProduct l, "\*", SAtom r;
SProduct => /(l, r) : SProduct l, "/", SAtom r;
SProduct => a : SAtom a;
```

Finally, we need to modify the productions for `SAtom`. The first one matches numbers. However,
since we bind the text that matched the regular expression to the variable `nr` it will have the
type `Str`, but we need to return an `Int`. Luckily, the `Str` class has a member called `toInt`
that performs the conversion for us. To call it, we can add `=> toInt(nr)` to the production as
follows:

```bnf
SAtom => toInt(nr) : "-?[0-9]+" nr;
```

The second and last production for handling parentheses is similar to the ones in `SExpr` and
`SProduct`. Since this production is only used to enforce the correct order of operations, it is
enough to simply return the value of `e`:

```bnf
SAtom => e : "(", SExpr e, ")";
```

To summarize, the grammar should now look like as follows:

```bnf
Int SExpr();
SExpr => +(l, r) : SExpr l, "\+", SProduct r;
SExpr => -(l, r) : SExpr l, "-", SProduct r;
SExpr => p : SProduct p;

Int SProduct();
SProduct => *(l, r) : SProduct l, "\*", SAtom r;
SProduct => /(l, r) : SProduct l, "/", SAtom r;
SProduct => a : SAtom a;

Int SAtom();
SAtom => toInt(nr) : "-?[0-9]+" nr;
SAtom => e : "(", SExpr e, ")";
```

We can now modify the `eval` function in Basic Storm to use the syntax transforms. This is done by
calling the `transform` member of the root node of the tree. This causes the transforms for the root
node to be evaluated. Since the root node uses the value of other nodes, this will cause the
relevant parts of the parse tree to be evaluated. Finally, we simply print the result:

```bs
void eval(Str expr) on Compiler {
    Parser<SExpr> parser;
    parser.parse(expr, Url());
    SExpr tree = parser.tree();
    Int result = tree.transform();
    print("${expr} evaluated to ${result}");
}
```

Experiment with a few different expressions to see how the system works. It might also be
interesting to add other operators to the syntax. For example, it is fairly easy to add `min(x, y)`
and `max(x, y)` to the grammar by adding new productions to the `SAtom` production.


Parameters in the Syntax
------------------------

So far, we have not needed to use parameters to productions in the syntax. To illustrate this, let's
add support for variables to our small language. The goal is to provide the syntax transforms with a
data structure, `VarList`, that contains a collection of named values that should be usable. This
allows writing expressions like: `a * 2`, or `x + y * 2`, for example.

First, we define a class that stores our variables:

```bs
class VarList {

    Str->Int values;

    void put(Str name, Int value) {
        values.put(name, value);
    }

    Int get(Str name) {
        if (x = values.at(name)) {
            x;
        } else {
            throw NoVariable(name);
        }
    }
}
```

As we can see, the class is essentially a thin wrapper around a regular `Map`. It would be
sufficient for our purposes to just use a `Map`, but creating a wrapper in this manner lets us
customize the `get` function to throw a nicer error, and makes it easier to extend it in the future
with custom operations that might be required by the syntax.

Since we throw a custom exception in the `get` function, we also need to define the exception class:

```bs
class NoVariable extends Exception {

    Str variable;

    init(Str variable) {
        init { variable = variable; }
    }

    void message(StrBuf out) : override {
        out << "No variable named " << variable << " is defined.";
    }
}
```

After doing this, we can start extending the grammar. To use variables we add a new production to
the `SAtom` rule:

```bnf
SAtom : "[A-Za-z]+" name;
```

This rule matches a string of one or more letters and binds it to the variable `name`. However, we
run into problems when trying to implement the transform for it. We wish to call the `get` function
of a `VarList` object, but we do not have access to such an object. To solve the problem, we can add
a parameter to the `SAtom` rule as follows:

```bnf
Int SAtom(VarList vars);
```

This means that whenever the transform for an `SAtom` is called, the caller needs to supply a list
of variables. This makes it possible to write the transform function for our new production as
follows:

```bnf
SAtom => get(vars, name) : "[A-Za-z]+" name;
```

This is good so far, but if we try to run the code at this point we will get an error that says:
"Can not transform a grammar.SAtom with parameters: ()". This means that we have not passed all
required parameters to the transform function in our grammar. This is indeed true, since we are
transforming `SAtom` in the productions for `SProduct` for example. To solve this problem, we need
to update our grammar so that both `SExpr` and `SProduct` accept a `VarList` as a parameter, and
passes it forward. To do this, add parentheses after the rule names in the productions and add the
name of the variable that shall be passed as a parameter. This makes the rules look like function
calls. After doing this modification, the grammar looks like this:

```bnf
Int SExpr(VarList vars);
SExpr => p : SProduct(vars) p;
SExpr => +(l, r) : SExpr(vars) l, "\+", SProduct(vars) r;
SExpr => -(l, r) : SExpr(vars) l, "-", SProduct(vars) r;

Int SProduct(VarList vars);
SProduct => a : SAtom(vars) a;
SProduct => *(l, r) : SProduct(vars) l, "\*", SAtom(vars) r;
SProduct => /(l, r) : SProduct(vars) l, "/", SAtom(vars) r;

Int SAtom(VarList vars);
SAtom => toInt(nr) : "-?[0-9]+" nr;
SAtom => e : "(", SExpr(vars) e, ")";
SAtom => get(vars, name) : "[A-Za-z]+" name;
```

Finally, we need to update the code in the `eval` function to pass a `VarList` object as a
parameter. To be able to test our implementation, we also add the variable `ans` to the list as
well:


```bs
void eval(Str expr) {
    Parser<SExpr> parser;
    parser.parse(expr, Url());
    SExpr tree = parser.tree();

    VarList variables;
    variables.put("ans", 42);
    Int result = tree.transform(variables);
    print("${expr} evaluated to ${result}");
}
```

Now, we can test the implementation by evaluating an expression like `1 + ans` by modifying `main`:

```bs
void main() {
    eval("1 + ans");
}
```


Transforms with Side-Effects
----------------------------

So far, we are only able to use pre-defined variables, not define new ones. Lets extend the language
further to make it possible to create new variables from within the language. The goal is to create
a language where we can specify one or more *statements* separated by commas. A statement is either
an assignment (`<variable> = <expression>`) or just an expression. The language evaluates all
statements in order, and records the value of all statements that were not assignments. For example,
the string:

```
a = 20, b = a - 10, a + b, b - a
```

Produces the result: `30, 10`.

To implement the grammar for this, we start by defining a new rule, `SStmt`, that accepts a variable
list like before. The rule has two productions, one for an assignment, and one for a plain
expression:

```bnf
void SStmt(VarList vars);
SStmt : SExpr(vars) expr;
SStmt : "[A-Za-z]+" name, "=", SExpr(vars) value;
```

We can then create a rule that matches a series of these values: `SStmtList`:

```bnf
void SStmtList(VarList vars);
SStmtList : SStmt(vars) - (, ",", SStmt(vars))*;
```

The production that matches the list might look a bit complex at first since it uses the repetition
syntax. The goal is to match `SStmt` one or more times, but with a comma between each occurrence.
First, it matches `SStmt` once, outside the repetition syntax. Then it specifies the `-` separator
since we do not want to match anything more before the parenthesis. The parenthesis is matched zero
or more times since it ends with a `*`. Each time it matches an optional whitespace, a comma,
another optional whitespace, followed by a `SStmt`.

To illustrate the behavior, let's expand the repetiton a number of times to see the pattern clearer:

```bnf
SStmtList : SStmt(vars);                            // Repeated 0 times
SStmtList : SStmt(vars), ",", SStmt(vars);                   // 1 time
SStmtList : SStmt(vars), ",", SStmt(vars), ",", SStmt(vars); // 2 times
// ...
```

The problem that remains is to write syntax transforms that implement our desired behavior. The
production for the assignment is the easiest one, so let's start there. For this production, we can
simply store the value of the new variable by calling `put` in our `VarList` class:

```bnf
SStmt => put(vars, name, value) : "[A-Za-z]+" value, "=", SExpr(vars) value;
```

The other productions are a bit trickier. The goal is to save the value of all non-assignment
expressions in an array. So let's start by updating the return type of `SStmtList`. Note that it is
*not* possible to use the shorthand `Int[]` in the Syntax Language:

```bnf
Array<Int> SStmtList();
```

Then, we need to write the the syntax transform in the `SStmtList` transform. We can start by simply
creating an array:

```bnf
SStmtList => Array<Int>() : SStmt(vars) - (, ",", SStmt(vars))*;
```

This will work, but the array will always be empty since we never added anything to it. One option
to do this would be to use the `->` syntax to call the `push` member of the array for each `SStmt`.
This can be done as follows:

```bnf
SStmtList => Array<Int>() : SStmt(vars) -> push - (, ",", SStmt(vars) -> push)*;
```

This does, however **not** work in this situation, as we only wish to add the non-assignment
statements. The rule above would add *all* statements. It would also require that the `SStmt` rule
would return an `Int`, which is not currently the case.

Instead, we can pass the array as a parameter to the `SStmt` rule, and let the productions there add
themselves to the array whenever it is suitable. As such, we modify the rule `SStmt` as follows:

```bnf
void SStmt(Array<Int> appendTo, VarList vars);
```

This means that we need to modify the `SStmtList` production to pass the array to the `SStmt` rule.
For this, we utilize the fact that the expression to be returned is bound as the variable `me` in
the rule. We can thereby use `me` to pass it to `SStmt`:

```bnf
SStmtList => Array<Int>() : SStmt(me, vars) - (, ",", SStmt(me, vars))*;
```

Finally, we need to actually add the result of expressions to the array. There are two ways in which
we can do this. The first one is to call `push` in the expression after the arrow as follows:

```bnf
SStmt => push(appendTo, expr) : SExpr(vars) expr;
```

Another option that abuses the semantics of the Syntax Language slightly is to utilize the fact that
`SStmt` returns `void`, and we may therefore bind anything to the variable `me` to be able to use
the `->` syntax. For our current purposes they are equivalent, but this approach has the benefit
that it is possible to call members multiple times in the same production. For example, this is
particularly useful when a production contains repetition:

```bnf
SStmt => appendTo : SExpr(vars) -> push;
```

In the end, the new grammar we added to the file is the following:

```bnf
void SStmt(Array<Int> appendTo, VarList vars);
SStmt => appendTo : SExpr(vars) -> push;
SStmt => put(vars, name, value) : "[A-Za-z]+" name, "=", SExpr(vars) value;

Array<Int> SStmtList(VarList vars);
SStmtList => Array<Int>() : SStmt(me, vars) - (, ",", SStmt(me, vars))*;
```


With the grammar done, the only remaining thing is to update the `eval` function once more to use
our revised syntax:

```bs
void eval(Str expr) on Compiler {
    Parser<SStmtList> parser;
    parser.parse(expr, Url());
    if (parser.hasError())
        throw parser.error();

    SStmtList tree = parser.tree();

    VarList variables;
    Array<Int> results = tree.transform(variables);
    print("Results: ${results}");
}
```

We can then modify `main` to verify that our implementation seems to work:

```bs
void main() on Compiler {
    eval("a = 20, b = a - 10, a + b, a - b");
}
```
