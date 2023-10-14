Expressions
===========

Basic Storm is an extensible language, and it is thus possible to develop language extensions for
it. This is done by creating a production that extends a suitable rule of the Basic Storm syntax. To
create new expressions, extend the rule `lang:bs:SExpr`. To create new statements (i.e. expressions
that do not need to end with a semicolon), extend the rule `lang:bs:SStmt`. Both of these rules are
expected to return a value of type `lang:bs:Expr`, which is the central class that represents an
expression in Basic Storm. As such, an extension may either create an abstract syntax tree that
consists of existing classes in Basic Storm, or create a new abstract syntax tree node that
implements the desired behavior. The latter is done by simply inheriting from the `Expr` class (or
any of its subclasses).

The `Expr` class have the following members that are relevant when developing an extension. Both
when implementing new expression classes, and when inspecting the abstract syntax tree:

- `result`

  Compute the result type of this expression, expressed as an `ExprResult`. The result may be either
  that the expression returns a value of a particular type, or that the expression never returns
  (e.g. by invoking `return` or `throw` on all paths).

- `code`

  Called by the parent block or expression to generate intermediate code for the expression. The
  first parameter is a `CodeGen` object that contains information about the code generation state.
  The second parameter `CodeResult` contains the location where the code should place the generated
  result.

  If `result` indicates that the expression returns a reference to a value, then the `code` member
  must be prepared to either generate a reference to a value, or to produce a non-reference.
  Expressions that report that they return a non-reference can assume that they will not be asked to
  return a reference.


- `castPenalty`

  Called to determine the penalty to cast the result of the expression into the type passed as a
  parameter. This interface is used when an exact match is not found, and some automatic type
  casting is necessary. Expressions may use this function to determine what type to return depending
  on the context in which they are used. For example, the `null` literal reports a result of `void`,
  since the exact type is not known. However, if it is used in a context where a `Maybe<T>` is
  expected, it will return a suitable type.

  Should return a non-negative value if the cast is possible. This means that the `code` function
  will properly handle receiving a result of the type passed to `castPenalty`. A negative result
  means that the cast is not possible.


The class contains a few additional members that are used internally. The ones mentioned above are,
however, the ones that are most relevant when developing extensions.
