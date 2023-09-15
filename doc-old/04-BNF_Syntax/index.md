BNF Syntax
============

This is the syntax language that is implemented in the compiler itself. It is used to implement the
syntax of [Basic Storm](md://Basic_Storm), and can also be used to extend it or implement new
languages. Please refer to [Syntax](md://Storm/Syntax) for an explanation of the syntax system in
the compiler and its semantics. This section will only discuss how syntax is represented using this
language.

Input
-------

To use the BNF Syntax language, create a text file with the extension `bnf`. BNF Syntax is read
using the standard text input in Storm, and therefore supports everything that is described
[here](md://Storm/Text_IO).

Name lookup and types
----------------------

The syntax language uses a `.` for separating names in types (in contrast to Basic Storm) and `<>`
to indicate parameters to a name part. When writing function calls `()` are used, just as in C. Note
that due to the implementation of names in Storm, it is not possible to combine `<>` with `()` like
this: `foo<Int>(a, b)`. There is an exception to this rule in constructor calls. Constructor calls
are written like: `Type(a, b)`. This really means `Type.__ctor(a, b)`, so in this special case,
`Type<Int>(a, b)` is perfectly legal, as it means `Type<Int>.__ctor(a, b)`.

Types are looked up in the following order in the BNF language:
1. the current package
2. any `use`d packages
3. the core package
4. the root package

The type `SStr` is always visible, even if the package `core.lang` is not visible.

Also note that, compared to Basic Storm, it is not possible to omit the parenthesis in a function
call in the syntax language. That makes the syntax language think you are referring to a variable
instead.

Extensibility
--------------

The syntax language may also be extended similarly to Basic Storm with some minor differences. In
the syntax language, it is more important to carefully consider which syntax is required in order to
be able to parse something else. Therefore, it is recommended to always create separate packages for
extensions to the syntax language in order to avoid any cycles that could cause strange behaviour.

For this reason, the syntax language uses a separate `use` statement for extending the syntax. `use`
only extends the namespace with more packages while the keyword `extend` only extends the set of
productions visible while parsing the syntax in the file. Just like in Basic Storm, `extend`
statements must be the first statements in the file. `use` statements can be located anywhere, even
if it is good practice to put them right after any `extend` statements.

Function calls
---------------

The syntax language is very limited in terms of combining functions. Each production has a
function call that generates the syntax tree for that production. This function call may also be a
variable in the production. Function calls take the form `<identifier>(<parameters>)`, where
`<parameters>` is a list of parameters separated by comma. Each parameter is either a variable, a
traversal or a literal. Function calls taking parameters are not allowed as parameters. Traversals
have the form 'x.y.z' and allow accessing members (such as member variables and member functions) in
addition to plain variables. The only type of literals currently implemented are numeric literals
and booleans, which are expanded to `core.Int` and `core.Bool` respectively.

Comments
---------

Comments can be included anywhere. Comments start with `//` like in C++, and continues to the end of
the line. C-style comments are not supported in the syntax language.

Visibility
-----------

All productions that can be resolved from the `use`d packages (in Basic Storm, any use statements in the
`.bs`-file are included) are visible and considered during the parsing process. Currently everything
declared in the syntax language is considered `public`.

Rules
------

Rules need to be declared. Declaring a rule takes the form:

`<result> <name>(<params>);`

Where `<params>` is a parameter list as it would be expressed in C. `<result>` is the return type of
the rule. The rule always lives in the package where the syntax file is located.

The result and the parameters are important for the [Syntax Transforms](md://BNF_Syntax/Syntax_Transforms)
later on.


Productions
------------

Productions (possible matches for a rule) are declared using the following syntax:

`<name> => <result> : <tokens>;`

__or__

`<name>[<priority>] => <result> : <tokens>;`

__or__

`<name> : <tokens>;`

__or__

`<name>[<priority>] : <tokens>;`


Where `<name>` is the name of the rule this production is a part of (fully qualified if
required), `<result>` is a function call or a variable name that contains the value that should be
returned from this production when matched. It can be omitted if the rule is declared to return
`void`. `<tokens>` is the sequence of tokens the production contains. The optional `<priority>` is a
number indicating the priority of this production. If it is left out, the production is given priority 0.
Higher priority is executed before rules with a lower priority, so rules with higher priority are
more greedy than ones with lower priority.

The tokens for the rule is a list of tokens separated with one out of three supported delimiters
(`-`, `,` or `~`, see *Delimiters* below). Each token is either:

* `<name>` - the name of another rule.
* `<name>(<params>)` - the name of another rule, giving parameters to the rule.
* `"regex"` - a regular expression.

Each token is optionally followed by an indication of what to do with the match. This is either just
a name, which means that the parser should bind the match to that variable, or an arrow (`->`)
followed by a name, which means that the parser should execute `me.<name>(<match>)` when the
function call in the rule has been executed.

By default the result of each rule is transformed before it is bound to a variable or sent to a
member function. If this is not desired (e.g. you want to capture parts of the syntax tree for later
transformation), append an `@` sign after the name or the regex like: `Foo@ -> bar`. This just
passes the raw syntax tree on to the function `bar` instead of transforming it first. Note that
because of this, it is useless to specify parameters along with `@`. When a `@` is appended to a
regex, the resulting type is `SStr` instead of `Str`. See [Syntax Transforms](md://BNF_Syntax/Syntax_Transforms)
for more details on syntax transforms.


Delimiters
----------

Each token is separated by one of three possible delimiters. These delimiters behave differently,
and are shorthands for different behaviors that are commonly used when writing grammars.

The dash (`-`) is the simplest of the separator. It is simply used to separate different tokens in
the grammar, and does not otherwise impact what is matched by the grammar. For example, in the
example below, the rules `A` and `B` are equivalent, and both match the string `ab`.

```
void A();
A : "a" - "b";

void B();
B : "ab";
```

The other two separators match a pre-defined rule whenever they appear in a production. The comma
(`,`) is intended for cases where whitespace is allowed, but not required. It is therefore called
the `optional delimiter`. In order to use it, it is necessary to specify which rule is to be used to
match whitespace in this particular language. This is done in a file-by-file basis using the
following syntax:

`optional delimiter = <rule name>;`

For example, the rules `A` and `B` below are equivalent, and both are equivalent to the regex `a[ \t]*b`:

```
optional delimiter = Whitespace;

void Whitespace();
Whitespace : "[ \t]";

void A();
A : "a", "b";

void B();
B : "a" - Whitespace - "b";
```

Finally, the tilde (`~`) works similar to the comma mentioned previously, but is instead intended
for cases where some amount of whitespace is required, and is called the `required delimiter`. It is
used in a similar fashion to the comma separator, as illustrated below. This example also attempts
to illustrate why it is useful to have the two separators by declaring the grammar for a simple
class declaration. Here, we want to allow `class A{}`, `class  A { }`, but not `classA{}`.

```
optional delimiter = Optional;
required delimiter = Required;

void Optional();
Optional : " *";

void Required();
Required : " +";

void ClassDecl();
ClassDecl : "class" ~ "[A-Za-z]+", "{", "}";
```

When writing syntax for a language, it is often convenient to include the comment syntax as a part
of the delimiter rules, which makes it possible to insert comments at any point in the grammar where
whitespace is allowed.

It is also possible to set both delimiters to the same rule in one go, by writing:

`delimiter = <rule name>;'

This is mostly intended for backwards compatibility with syntax written before the required
delimiter was introduced.


Repitition
-----------

It is possible to express repetition only using rules and productions, but many times it is clearer to
write the repetitions explicit. The syntax language has support for the same repetitions present in
regular expressions: `*`, `?` and `+`. To use them, enclose the tokens that are to be repeated in
parenthesis and add the desired repetition after the closing parenthesis, like this:

```
void A();
A : "a", ("b",)* "c";
```

Which will match `a`, followed by zero or more `b`:s and finally a `c`. Each character is separated
by the delimiter rule (not specified for this example). Note that parenthesis can be either before
or after separators. There may even be separators on both side of the parenthesis. Currently, only
one repetition per production is supported. Break the production into multiple rules if more repeats are
needed. Due to the nature of repetitions, it is not practical to bind any matches inside a repeat to
a variable. Use the `->` syntax inside repetitions instead. What happens if you try to bind tokens
inside a repetition to a variable is that the variable will either not have a value, or be assigned
multiple times. Both of these cases will generate an error when the productions are being
evaluated. This could be solved by adding support for array-variables, but implementing
array-variables in the syntax would significantly increase the complexity of the rules for
variables. Usually it is also more readable to generate variables explicitly by creating a new rule
for the repetition, which creates an array:

```
Array<T> List();
List => Array<T>() : (Rule -> push, )*;
```

Aside from repetitions, parenthesis can also be used to capture the string matched by a set of
tokens. This can not be used together with repetitions, since it also uses parenthesis. The syntax
is as follows:

`<tokens> ( <tokens> ) <name> <delimiter> <tokens>`

For example, to get the string within a pair of brackets (`{}`), assuming we want the string all the
way to the next matching closing bracket, we can do like this:

```
void Match();
Match : "[^{}]";
Match : Match - Match;
Match : "{" - Match - "}";

Str Content();
Content => v : "{" - (Match) v - "}";
```

In this example, the rule `Match` does not return anything. Instead, we are binding whichever
characters `Match` matched to the variable `v` and return it. Of course, more than one token can be
included inside the parenthesis. This is very useful when matching complex identifiers, or skipping
parts of the input for parsing later on, possibly with a different syntax. The `->` and `@` syntax
is also supported with captures.

Context sensitivity
-------------------

Storm supports a simple form of context sensitive syntax in order to improve extensibility of
grammars from different origins without causing different extensions to accidentally interfere with
each other. Consider the following case: You want to specialize a syntax extension that provides
some domain specific syntax. You do this by adding productions to one or more of the rules in the
extension. However, your additions only make sense when the extension is used in your specific
context, so you want to make sure that the extension is not used outside of the expected
context. This is possible with context free grammars, but it is cumbersome as it requires
duplicating some of the productions inside the extension. For this reason, Storm allows restricting
the use of certain productions to certain contexts by specifying that a rule needs to have a
particular (direct or indirect) parent.

A production may be declared to be usable only in a certain context by prepending the name of a rule
followed by two dots to the production declaration, as for the last production in the following
example:

```
void Root();
Root : A;
Root : B;

void A();
A : "a" - B;

void B();
B : "b";
A..B : "c";
```

In this case, the last production is only usable if the rule `A` is a direct or indirect parent to
an application of the rule `B`. Therefore, the string `c` is not matched by this grammar, but the
string `ac` is. Thus, `c` is only usable in the context of the rule `A`.

While these requirements generally do not incur a great performance penalty, the parser in Storm is
implemented based on the assumption that the number of unique rules used as requirements to
productions (i.e. appearing to the left of the dots) is fairly small. If many requirements and many
unique rules are used, parsing performance could suffer, even though it should not be much worse
than parsing and resolving the ambiguities in the context free grammar created by removing all
context sensitivity.

A concrete example of the usefulness of this ability is the implementation of patterns in Basic
Storm. This extension introduces a new block to Basic Storm, which can contain arbitrary Basic Storm
expressions. However, inside this block it is also possible to use the syntax `${...}`, which only
makes sense inside these blocks. In order allow other extensions to be used inside the blocks, and
to avoid duplicating the grammar of Basic Storm, the pattern block can contain any expressions
described by the regular `SExpr` rule, and the additional syntax is added by an additional
production to `SAtom`.

Without context sensitivity, the new syntax would be usable outside the pattern blocks, meaning that
the new syntax needs to check for the proper context manually. Furthermore, this could cause
problems when using other extensions that potentially use the same syntax for something else. In
this case, Storm does not define which extension takes precedence (unless priorities are used),
which could result in unexpected results. This problem is easily solved by making the new production
usable only in the context of the pattern block, much like the last production in the example above.


Syntax Highlighting and Indentation
====================================

The syntax language allows specifying annotations that are used by the language server to provide
syntax highlighting and automatic indentation of the code to users of a language. These are
discussed in more detail in [Syntax Highlighting](md://BNF_Syntax/Syntax_Highlighting) and
[Indentation](md://BNF_Syntax/Indentation).

Examples
=========

The example *Eval* on the [Examples](md://Introduction/Examples) page shows how to use the syntax
language together with Basic Storm to create a simple expression evaluator.
