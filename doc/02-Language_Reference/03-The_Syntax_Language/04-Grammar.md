Syntax
======

As mentioned in the section on [terminology](md:Terminology), context-free grammar consists of rules
(usually called nonterminals), productions, and terminals. This section will cover their syntax in
the Syntax Language, and the languages they match.

Formally, a language is a set of strings that conforms to a given specification. In the context of
context-free languages, the specification can be written using a set of rules and productions. All
strings in the language can then be constructed by starting at the *starting rule*, and applying
productions until all rules have been turned into nonterminals. Since there are typically many
possible productions that can be applied at each step, context-free languages typically define very
large languages. In Storm, the start rule is not defined in the grammar, but instead when a parser
is instantiated.

One may also think of grammars in reverse. One can think of the parser as starting by trying to
match the start rule. It attempts to match each production in turn until it finds one that matches a
part of the input string. It then continues to match any rules in the matched production until the
entire input string is matched by the grammar. Of course, this would be an inefficient way to
implement a parser, but the rough idea is not too far from the thruth.

In general, the order of declarations in the Syntax Language is not important. There are, however,
two exceptions to this. First, any `use` or `extend` statements must appear first in the file.
Similarly, declarations for the meaning of delimiters must appear before the delimiter is used.

Comments
--------

The Syntax Language supports single-line comments. These start with `//` and end at the end of the
line. Multi-line comments are not supported.


Use Statements
--------------

Use statements have the form: `use <name>;`. The name is expected to refer to a package. The use
statement has the effect that the package `<name>` will be searched for any rules, types, or
functions that are referred in the file. For example, if the type `lang.bs.SExpr` will be used
frequently, including `use lang.bs;` at the top of the file makes it possible to refer to the rule
by simply writing `SExpr` instead of the fully qualified name.

Extend Statements
-----------------

Extend statements have the form: `extend <name>;`, where `<name>` refers to a package. This
statement has the effect that the *syntax* in the package `<name>` will be used when parsing the
remainder of the file. As such, the `extend` statement is used to load *syntax extensions* to the
Syntax Language itself.

Since the Syntax Language is parsed to extract syntax from a package, some care need to be taken
with syntax extensions to the Syntax Language in order to avoid circular dependencies. For this
reason it is best to always place syntax extensions to the Syntax Language in a separate package
from where it is used.


Rules
-----

Storm, and thus the Syntax Language, require rules to be defined. The main reason for this is to
implement type-safe [syntax transforms](md:Syntax_Transforms), but it is also helpful in avoiding
typos. As we shall see, the syntax transformations treat rules similarly to functions. As such,
rules are defined using a syntax similar to functions in Basic Storm:

```
<result> <name>(<parameters);
```

Where `<result>` is the name of the type returned by the transform function, `<name>` is an
identifier that names the rule, and `<parameters>` is a comma-separated list of formal parameters to
the transform function. As in [Basic Storm](md:../Basic_Storm/Definitions/Functions), each parameter
consists of the type of the parameter followed by the name of the parameter. As such, a rule
definition might look as follows:

```bnf
core.Array<Int> SIntArray(Int first);
```

Since we will not introduce syntax transforms until [later](md:Syntax_Transforms), we will define
all rules as accepting no parameters, and returning `void` for the time being:

```bnf
void SMyRule();
```

As we can see, most of the libraries in Storm follow the convention to name rules starting with an
uppercase `S`. This is not enforced by Storm, but since rules become visible in the name tree as
types, it helps to make the distinction between the syntax for a concept, and the actual
implementation of the concept. As an example, the rule that matches expressions in Basic Storm is
named `SExpr` to distinguish it from the class that implements expressions, called `Expr`.


Productions
-----------

Productions define the actual language by defining one way in which a rule can be replaced with a
sequence of other rules and/or nonterminals. Productions are thus where the language itself is
defined. Ignoring syntax transforms, rules are defined as follows:

```
<rule name>[<priority>] : <tokens> = <name>;
```

Above, `<rule name>` is the name (possibly qualified) of a rule that this production extends. The
part `<priority>` optionally specifies the priority of the production to resolve any ambiguities in
the syntax. Note that the square brackets are a part of the syntax, and they need to be included if
`<priority>` is specified. `<tokens>` is a list of zero or more tokens that the rule may be expanded
to. Finally, `<name>` is the optional name of the production. If the name is not included, the equal
sign `=` before it is also omitted. Productions typically only need a name if other code needs to
manipulate the parse tree, and it is therefore usually omitted.

The list of tokens (`<tokens>`) is a list of tokens separated by delimiters. These are covered
below.


Tokens
------

There are two types of tokens: terminal tokens and rule tokens. Terminal tokens can be thought of as
literal strings that appear in the input. They are, however, regular expressions that are matched to
the input. They are therefore often referred to as *regex tokens*. Rule tokens simply refer to a
rule to indicate that they can be substituted by any production in the referred rule.

### Regex Tokens

A regex token is written in double quotes (`"`), similarly to a string literal in Basic Storm. The
string is expected to contain a regular expression. In particular, the following elements of regular
expressions are supported:

- `.` - match any single character.
- `[abc]` - match any one character in the group.
- `[^abc]` - match any one character *not* in the group.
- `[a-c]` - match characters between `a` and `c` (enumerated as codepoints).
- `[a-ce-g]` - match characters between `a` and `c`, as well as between `e` and `g` (enumerated as codepoints).
- `?` - match the previous character zero or one times.
- `*` - match the previous character zero or more times.
- `+` - match the previous character one or more times.

It is possible to escape any character with a special meaning to match that character, including the
double qoute that would end the literal.

For example, a rule that matches a number could look as follows:

```bnf
SNumber : "[0-9]+";
```

It is worth noting that regular expressions are not matched as a single unit, opaque to the parser.
This means that regular expressions are always greedy. For example, the following production would
never match, since the first regex token will always consume all available `a`:s:

```bnf
SNoMatch : "a*" - "a";
```

It will, however, work properly the token repetition syntax (see below) is used:

```bnf
SMatch : ("a")* - "a";
```


### Rule Tokens

Rule tokens are simply written as the (possibly qualified) name of a rule in the system. For
example, another way to match numbers is to use recursion to match a single digit at a time:

```bnf
void SNumber();

SNumber : "[0-9]" - SNumber; // Match a single digit followed by a number.
SNumber : "[0-9]";           // ...or just a single digit.
```

Delimiters
----------

Each token is separated by one of three possible delimiters. While all of them have the role of
separating different tokens, they have different meanings and are useful in different situations.

The dash (`-`) is the simplest of the separators. It simply separates different tokens in a
production, and does not impact the meaning of the rule. For example, the two productions below are
equivalent and both match the string `ab`:

```bnf
void A();
A : "a" - "b";

void B();
B : "ab";
```

The other two separators match a pre-defined rule whenever they appear in a production. Since there
is no tokenizer in Storm, it is a common operation to match a rule that matches optional- and
required whitespace. As such, the two other separators give this common operation a convenient
shorthand.

The comma separator (`,`) is intended for cases where whitespace is allowed but not required. As
such, it is called the *optional delimiter*. In order to use it, it is necessary to specify which
rule shall be used to match optional whitespace. This is done in the top of the file as follows:

```
optional delimiter = <rule name>;
```

For example, the rules `A` and `B` below are equivalent. Both are equivalent to the regular
expression `a[ \t]*b` (i.e. `a` and `b` separated by zero or more tab- or space characters):

```bnf
optional delimiter = Whitespace;

void Whitespace();
Whitespace : "[ \t]*";

void A();
A : "a", "b";

void B();
B : "a" - Whitespace - "b";
```

The final separator, the tilde (`~`) is intended to be used to match cases where at least some
whitespace is required. It is therefore called the *required delimiter*. The rule matched by the
required delimiter is specified as follows:

```
required delimiter = <rule name>;
```

For example, it can be used to match simple class definitions as follows. The rule `ClassDecl` in
the grammar below matches `class A { }`, `class A{}`, and `class A {}`, but not `classA{}`:

```bnf
optional delimiter = Optional;
required delimiter = Required;

void Optional();
Optional : " *";

void Required();
Required : " +";

void ClassDecl();
ClassDecl : "class" ~ "[A-Za-z]+", "{", "}";
```

The rule `ClassDecl` can also be written as follows:

```bnf
void ClassDecl();
ClassDecl : "class" - Required - "[A-Za-z]+" - Optional - "{" - Optional - "}";
```


Repetition
----------

It is possible to express repetition in context-free grammars using only recursion. The Syntax
Language does, however, provide the ability to express some common repetition patterns more
compactly.

Tokens in a production can be repeated by surrounding them in parentheses followed by a character
that indicates how the contents of the parantheses could be repeated. Supported characters are `?`,
`+`, and `*`. They have the same meaning as in the regular expressions described above.

For example, it is possible to match a block of zero or more statements as follows (assuming
that delimiters are defined, and that the rule `SExpr` exists).

```bnf
void SBlock();
SBlock : "{", (SExpr, )* - "}";
```

Note that the comma separator is placed inside the parenthesis. This is not required by the syntax
language, but it means that the optional separator will be a part of the repetition. Separator may
also appear after the start parenthesis, but before the first token.

To illustrate the benefits of the repetition syntax, the rule above could also be defined as
follows:

```bnf
void SBlock();
SBlock : "{", SBlockRep - "}";

void SBlockRep();
SBlockRep : SExpr, SBlockRep;
SBlockRep : ; // Match the empty string.
```

It is only possible to have a single pair of parentheses in each production.



Production Priorities
---------------------

The parsers designed to parse source code provided by Storm support *ambiguous grammars*. While this
makes parsing a bit more difficult, the upside is that allowing ambiguous grammars makes it possible
to compose grammars for different languages easily (i.e. context-free languages are closed under
composition if ambiguous grammars are allowed). This does, of course, introduce some problems.
Namely, it is necessary to disambiguate any ambiguities in a sensible way. Storm solves the problem
by using priorities for productions. A production with higher priority is selected before a
production with lower priority. Priorities are resolved as if the grammar was parsed using a
recursive descent parser.

For example, priorities can be used to disambiguate the following ambiguous grammar to prefer the
first match for the string `aa`:

```bnf
void SRule();
SRule[4] : "aa";
SRule[1] : "a.";
```


Context Sensitivity
-------------------

Storm supports a simple form of context sensitive syntax in order to improve extensibility of
grammars from different origins without causing different extensions to accidentally interfere with
each other.

A concrete example of where this ability is used is in the implementation of
[patterns](md:../Basic_Storm/Extensibility/Metaprogramming) in Basic Storm. This extension
introduces a new block to Basic Storm, which can contain arbitrary Basic Storm expressions. However,
inside this block it is also possible to use the syntax `${...}`, which only makes sense inside
these blocks. In order allow other extensions to be used inside the blocks, and to avoid duplicating
the grammar of Basic Storm, the pattern block can contain any expressions described by the regular
`SExpr` rule, and the additional syntax is added by an additional production to `SAtom`.

Without context sensitivity, the new syntax would be usable outside the pattern blocks, meaning that
the new syntax needs to check for the proper context manually. Furthermore, this could cause
problems when using other extensions that potentially use the same syntax for something else. In
this case, Storm does not define which extension takes precedence (unless priorities are used),
which could result in unexpected results. This problem is easily solved by making the new production
usable only in the context of the pattern block, much like the last production in the example above.


Storm solves this problem by allowing productions to be specified as only applicable if another rule
has appears as a parent in the parse tree. This is done by prepending the name of a production
followed by two dots to the start of the production definition:

```bnf
void Root();
Root : A;
Root : B;

void A();
A : "a" - B;

void B();
B : "b";
A..B : "c";
```

In this example, the last production is only usable if the rule `A` is a direct or indirect parent
to an application of the rule `B`. Therefore, the string `c` is not matched by this grammar, but the
string `ac` is. Thus, `c` is only usable in the context of the rule `A`.

While these requirements generally do not incur a great performance penalty, the parser in Storm is
implemented based on the assumption that the number of unique rules used as requirements to
productions (i.e. appearing to the left of the dots) is fairly small. If many requirements and many
unique rules are used, parsing performance could suffer, even though it should not be much worse
than parsing and resolving the ambiguities in the context free grammar created by removing all
context sensitivity.

