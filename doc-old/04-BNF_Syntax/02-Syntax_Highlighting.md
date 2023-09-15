Syntax Highlighting
=====================

It is possible to use the language server in Storm to highlight source files in languages supported
by Storm. This is done by annotating the productions with information that indicates which parts of
the syntax to highlight.

Highlighting information can be specified for each token in a production, or when defining a
rule. This is done by adding a number sign (`#`) followed by the name of a color to a token. This
information should always be the last part of the token declaration. The following colors are
supported:

* `none`: no color. Clears any previous coloring for the specified range.
* `comment`: used for comments.
* `delimiter`: used for delimiters.
* `string`: used for string literals.
* `constant`: used for other constants and literals, for example numbers.
* `keyword`: used for keywords in the language.
* `fnName`: used for names of functions.
* `varName`: used for names variables.
* `typeName`: used for names of types.

As can be seen from these colors, Storm is not concerned with the actual color of the text. Instead,
it reports the syntax class to the editor being used and lets the editor decide which color to
use. This makes Storm respect the color theme in use by the editor.


Annotations containing syntax highlighting information can either be placed after a token in a
production as follows:

```
VarDecl : "var" #keyword, "[A-Za-z]+" #varName;
```

This means that the characters matched by the first regular expression shall be colored as a keyword
while the characters matched by the second regular expression are colored as a variable name. If
multiple highlighting is present for some characters, the annotation closest to the root rule takes
precedence. So, if we use `VarDecl` in a different situation, we can override the highlighting as
follows:

```
X : VarDecl #constant;
```

Aside from declaring coloring for individual tokens, it is possible to set the coloring for entire
rules at once. This is done as follows:

```
void Modifier() #keyword;
Modifier : "public";
Modifier : "private";
```

Doing this is equivalent of adding the `#keyword` annotation everywhere `Modifier` is used as a
token. This can be overridden by adding another modifier when using the rule, as follows:

```
X : Modifier; // This is equivalent to X : Modifier #keyword;
Y : Modifier #varName; // This overrides #keyword declared on the rule.
```
