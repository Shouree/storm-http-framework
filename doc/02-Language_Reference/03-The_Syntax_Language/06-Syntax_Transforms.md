Syntax Transforms
=================

Using the representation of the parse tree, it is possible to traverse it to transform it into an
abstract syntax tree. For example, it could be done similarly to this in Basic Storm:

```bs
Expr transformExpr(SExpr expr) {
    if (expr as SAssignExpr) {
        return AssignExpr(expr.lhs, expr.rhs);
    } else if (expr as SCallExpr) {
        return CallExpr(expr.function, expr.params);
    }
    // many more cases
    else {
        throw InternalError("Unknown expression type.");
    }
}
```

While this approach would work, it has three main problems:

1. It is tedious to write code like above.
2. It is tightly coupled with the shape of the grammar.
3. It is necessary to modify the function in order to add semantics from new grammar.


To address these problems, the Syntax Language provides annotations that are able to generate
functions like the one above automatically. Since the annotations are provided together with the
grammar, it is possible for syntax extensions to provide both new grammar along with the transforms
that hook into the system and implement the semantics of the new grammar.

As we shall see, syntax transforms are fairly limited in what they can do. They are, however,
powerful enough to do things like evaluating simple arithmetic expressions.

Transform Functions
-------------------

The goal of syntax transforms is to generate a member function called `transform` that is present in
all nodes of the parse tree. The `transform` function can then be called to transform the parse tree
into an abstract syntax tree according to the annotations in the grammar.

The [rule definition](md:Syntax) specifies the signature of the `transform` function generated in
the type generated for the rule itself, and for the types of any productions that extend the rule.
For example, consider the rule below:

```bnf
Int A(Str param);
```

The type that corresponds to the rule itself contains an abstract `transform` function with a
signature that matches the rule, as follows:

```bs
class A extends lang:bnf:Node {
    Int transform(Str param) : abstract;
}
```

Any types that correspond to productions that extend the rule will contain a similar `transform`
function that overrides the one in the base class and evaluates the transforms for that particular
rule.


The Body of a Transform Function
--------------------------------

The body of a transform function is generated in three steps. First, the *return value* specified in
the production is evaluated. Then, any captures specifed with the *member call* syntax are evaluated
in order. Finally, any remaining rule tokens to which parameters were specified are evaluated.

The *return value* is specified between the rule name and the list of tokens in a production. All
productions that extend a rule declared as returning anything other than `void` needs to specify a
return value. It is specified as follows:

```
<rule name> => <return value> : <tokens>;
```

The return value is not an arbitrary expression. It is limited to one of the following forms:

- A single identifier. The name should refer to either a formal parameter in the rule, or a captured
  token in the production.

- A literal. Integer and boolean literals are supported.

- A function call. That is, a [name](md:Names) followed by a set of parentheses that contain a list
  of parameters. Each parameter may be a single identifier (as above), a literal (as above), or a
  *traversal*. A *traversal* is a sequence of identifiers that can be used to traverse an object
  hierarchy.


As previously mentioned, the first part of the transform function evaluates the return value, which
might involve evaluating the value of captured parts of the parse tree. Any captured parts of the
parse tree are transformed by calling their respective transform function before they are used.

When the return value is evaluated, it is bound to a variable named `me`. This variable is then used
to execute any *member calls* in the production. Member calls are specified similarly to captured
tokens, by appending an arrow (`->`) followed by an identifier to a token. This identifier is
expected to refer to a member function of the type of `me`. The transform function then calls the
specified member with the transformed token (i.e. `me.<identifier>(<token>)`).

Similarly to captured tokens, tokens that have *member calls* will also be present in the parse
tree. However, since it is possible to call the same member functions multiple times, the variables
in the parse tree will be named `<identifier>0`, `<identifier>1`, etc.


Since the transform function transforms any captured tokens before they are used, it is necessary to
specify parameters to the transform functions of any captured rule tokens that require parameters.
This is done by appending parentheses after a token that specifies the list of parameters to pass to
the function. These parameters may take the same form as the parameters to a function call that
appears in the return value position of the production.

Since the transform functions of productions may have side-effects (e.g. appending elements to a
data structure that is passed as a parameter), the final step of the transform function is to
execute the transform functions of any captured rule tokens that were given parameters. To be able
to access these tokens, they are also captured in the parse tree, but with a name that is generally
not accessible to languages.


### Example 1 - Matching substrings

The first example uses the syntax transforms to find and return the string that is inside a set of
nested parentheses:

```bnf
Str FindParens();
FindParens => found : "(" - FindParens found - ")" = Recursive;
FindParens => found : "[a-z]+" found = Base;
```

The grammar recursively matches parentheses until it reaches the base case. At that point, it
captures the string, binds it to the variable `found`, and returns it all the way to the root of the
parse. This grammar generates types similar to below:

```bs
class FindParens extends lang:bnf:Node {
    Str transform() : abstract;
}

class Recursive extends FindParams {
    FindParens found;

    Str transform() {
        Str me = found.transform();
        return me;
    }
}

class Base extends FindParams {
    core:lang:SStr found;

    Str transform() {
        Str me = found.transform();
        return me;
    }
}
```

Note that the names for productions are not necessary. They are only specified here to clarify which
type belongs to which production.

To run the code, it is necessary to create a parser to parse the string:

```bs
use core:lang;
use core:io;

Str findParens(Str input) on Compiler {
    // Create a parser, the parameter specifies the start rule.
    Parser<FindParens> parser;
    parser.parse(input, Url()); // Url() gives the location of the file, used in error messages.
    // Check if there are any errors. Otherwise, the parser might
    // not have matched the entire input.
    if (parser.hasError)
        parser.throwError();
    // Create the parse tree.
    FindParens tree = parser.tree();
    // Transform it.
    return tree.transform();
}
```


### Example 2 - Removing characters

The next example uses grammar and transform functions to remove all occurrences of the letter `a`
in a string. For this, we will rely on the *member call* construct to call the `add` member of the
string buffer class, `StrBuf`. This can be implemented entirely in the following grammar:

```bnf
StrBuf RemoveA();
RemoveA => StrBuf() : ("[^a]*" -> add - "a")* - "[^a]*" -> add = Impl;
```

The grammar above matches a string that does not contain `a`, followed by a single `a`. This
sequence is repeated zero or more times to allow matching strings containing any number of the
letter `a`. Finally, the string may end with a string that does not contain the letter `a`.

Transform functions are used to reconstruct the parts that did not contain `a`. The return value is
specified as `StrBuf()`. This means that the transform function will call the constructor of
`StrBuf`, which creates an instance of the class. It will then traverse the match from left to right
and call the `add` member of `StrBuf` to add the transformed regex tokens, which will each have the
type `Str`. The transform functions will thus generate code similar to below:

```bs
class RemoveA extends lang:bnf:Node {
    StrBuf transform() : abstract;
}

class Impl extends RemoveA {
    core:lang:SStr[] add0;
    core:lang:SStr add1;

    StrBuf transform() {
        StrBuf me = StrBuf();
        for (Nat i = 0; i < this.add0.count; i++) {
            Str add0 = this.add0[i].transform();
            me.add(add0);
        }
        Str add1 = this.add1.transform();
        me.add(add1);
        return me;
    }
}
```

To run the code, it is necessary to create a parser to parse the string:

```bs
use core:lang;
use core:io;

Str removeA(Str input) on Compiler {
    // Create a parser, the parameter specifies the start rule.
    Parser<RemoveA> parser;
    parser.parse(input, Url()); // Url() gives the location of the file, used in error messages.
    // Check if there are any errors. Otherwise, the parser might
    // not have matched the entire input.
    if (parser.hasError)
        parser.throwError();
    // Create the parse tree.
    RemoveA tree = parser.tree();
    // Transform it.
    return tree.transform();
}
```

### Example 3 - Removing characters using recursion

We can rewrite the grammar from the example above using recursion. This makes it necessary to create
the `StrBuf` in the root production, and then pass it to other productions so that they can add
substrings to it. This example thus illustrates that transforms are evaluated for side effects, how
parameters are passed to productions, and that it is possible to use the return value even for rules
that return `void`.

```bnf
StrBuf RemoveA();
RemoveA => StrBuf() : RemoveImpl(me) = Impl;

void RemoveImpl(StrBuf to);
RemoveImpl => to : RemoveImpl(to) - "a" - "[^a]*" -> add = Rec;
RemoveImpl => to : "[^a]*" -> add = Base;
```

It generates the following types. Note that the names of the captured rule tokens have names that
are not valid in Basic Storm (by design). The code below will therefore not compile, and is only
intended to illustrate how the syntax transforms operate:

```bs
class RemoveA extends lang:bnf:Node {
    StrBuf transform() : abstract;
}

class Impl extends RemoveA {
    RemoveImpl anon0;

    StrBuf transform() {
        StrBuf me = StrBuf();
        anon0.transform(me);
        return me;
    }
}

class RemoveImpl extends lang:bnf:Node {
    void transform(StrBuf to) : abstract;
}

class Rec extends RemoveImpl {
    RemoveImpl anon0;
    core:lang:SStr add1;

    void transform(StrBuf to) {
        StrBuf me = to;
	    anon0.transform(to);
        Str add0 = this.add0.transform();
        me.add(add0);
    }
}

class Base extends RemoveImpl {
    core:lang:SStr add0;

    void transform(StrBuf to) {
        StrBuf me = to;
        Str add0 = this.add0.transform();
        me.add(add0);
    }
}
```




Capturing the Raw Parse Tree
----------------------------



Extending the Parse Tree
------------------------