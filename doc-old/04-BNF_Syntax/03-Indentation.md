Indentation
============

In addition to [Syntax Highlighting](md://BNF_Syntax/Syntax_Highlighting), the language server is
also able to assist the user indenting code. Storm supports two different kinds of indentation:
indentation by levels and indentation relative to other characters in the source. Level-based
indentation is simply represented by an integer that indicates how many tabs or spaces (or any other
measurement, this is decided by the editor being used) the line should start with. Indentation as
another character causes Storm to point to another character already in the source code, and tell
the client to indent the current line to the same level as that character.

When the editor wishes to indent a line, it requests the indentation for the first non-whitespace
character of that line. Storm then traverses the syntax tree of the parsed string (or equivalently,
the grammar) from the leaf node containing the character requested by the editor (the regular
expression) towards the start production of the grammar. At each step, Storm examines the production
to see if the token containing the requested character lies within the specified indentation
range. If that is the case, Storm applies the specified indentation action. The indentation actions
are as follows:

* `+` increases the indentation level one unit unless `@` or `$` has been previously applied.
* `-` decreases the indentation level one unit unless `@` or `$` has been previously applied.
* `?` increases the indentation level one unit if no other actions affecting indentation levels
      have been seen so far.
* `@` indicates that the current line shall be indented as the first character in the token
      before the range.
* `$` indicates that the current line shall be indented as the first character after the token
      before the range.

In the grammar, the indentation range and action are specified by enclosing one or more tokens in
square brackets (`[]`), followed by the desired indentation action. For example, in Java-like
languages, blocks are typically implemented as follows:

```
Expr : "{" [, (Expr, )* ]+ "}";
```

This causes the contents of the block to be indented by one additional level compared to the code
outside of the block.

The `?` action can be used to properly indent `if` statements without a block as follows:
```
Expr : "if", "(", Expr, ")" [, Expr]?;
```

This causes the expression after the if statement to be indented by one additional level unless the
production matched there contains an indent action. If the block production previously mentioned was
matched, Storm will have seen other indentation action (even if they are not applied), and therefore
disregards the `?` action. In case another expression is matched, it does not contain `+` or `-`
actions, and therefore the `?` is applied as desired.

`@` and `$` actions can be used for aligning parameters to functions as follows:

```
Fn : "[A-Za-z]", "(" [, Params, ")"]$;
```
