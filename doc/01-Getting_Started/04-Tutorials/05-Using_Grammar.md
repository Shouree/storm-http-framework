Using Grammar in Storm
=====================

This tutorial shows how to use [the Syntax Language](md:/Language_Reference/The_Syntax_Language)
together with Basic Storm to parse simple strings. As with the other tutorials, the code produced by
this tutorial is available in `root/tutorials/grammar`. You can run it by typing
`tutorials:grammar:main` in the Basic Storm interactive top-loop.

Setup
-----

First, we need somewhere to work. Create an empty directory somewhere on your system. The name does
not matter too much, but it is easier to avoid spaces and numbers in the name since it will be used
as a package name in Storm. For example, use the name `grammar`. Then, open a terminal and change
into the directory you just created.


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


Let's use transforms instead! Remove the names and add transforms!


```bnf
Int SExpr();
SExpr => p : SProduct p;
SExpr => +(l, r) : SExpr l, "\+", SProduct r;
SExpr => -(l, r) : SExpr l, "-", SProduct r;

Int SProduct();
SProduct => a : SAtom a;
SProduct => *(l, r) : SProduct l, "\*", SAtom r;
SProduct => /(l, r) : SProduct l, "/", SAtom r;

Int SAtom();
SAtom => toInt(nr) : "-?[0-9]+" nr;
SAtom => e : "(", SExpr e, ")";
```


```bs
void eval(Str expr) on Compiler {
    Parser<SExpr> parser;
    parser.parse(expr, Url());
    SExpr tree = parser.tree();
    Int result = tree.transform();
    print("${expr} evaluated to ${result}");
}
```

Try with some different expressions.


Parameters in the Syntax
------------------------

Let's add variables!

First, we add a class for our variables with associated exception:

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

void eval(Str expr) {
    Parser<SExpr> parser;
    parser.parse(expr, Url());
    SExpr tree = parser.tree();

    VarList variables;
    variables.put("ans", 42);
    Int result = tree.transform(variables);
    print("${expr} evaluated to ${result}");
}

void main() {
    eval("1 + ans");
}
```

And modify the rules (likely, step-by-step):

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

Transforms with Side-Effects
----------------------------

Now, we can add support for assignments!

```bnf
void SStmt(Array<Int> appendTo, VarList vars);
SStmt => appendTo : SExpr(vars) -> push;
SStmt => put(vars, name, val) : "[A-Za-z]+" name, "=", SExpr(vars) val;

Array<Int> SStmtList(VarList vars);
SStmtList => Array<Int>() : SStmt(me, vars) - (, ",", SStmt(me, vars))*;
```

Changes to the BS file:

```bs
void eval(Str expr) {
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

For input `eval("ans = 42, t = ans * 2 + 1, u = ans + t, ans, t, u")` outputs: `42, 85, 127`
