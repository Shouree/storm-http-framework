Blocks
======

Another central concept when developing extensions is the concept of a block. In source code, a
block contains both a set of local variables, and a sequence of expressions. In the abstract syntax
tree, these two concepts are separate. The class `lang:bs:Block` is an expression that only contains
a set of variables that are visible inside the block. This class therefore focuses on managing the
variables and their lifetimes. Below is a summary of the most relevant members in the `Block` class:


- Constructor

  Since blocks deal with name lookup, they need to know their parent scope. This is achieved by
  either supplying a `Scope` that represents the parent scope, or a reference to another block. The
  former is typically used to create the root block in a function, and the latter is typically used
  inside functions to create nested blocks.

- `lookup`

  This variable contains a `BlockLookup` that inherits from `core:lang:NameLookup`. This class thus
  represents a node linked to the name tree, so that it is possible to create a `Scope` instance
  that represents the variables visible inside a block in Basic Storm code. Note that it is only
  possible to traverse from the `BlockLookup` and upwards in the name tree: there are no pointers
  from the function entity itself into all blocks. The blocks only exist briefly, during compilation
  of a function.

- `scope`

  Contains a `Scope` that refers to the current block. It is possible to create a `Scope` from the
  `lookup` member, but it is provided as a convenience.

- `blockCode`

  When overriding a `Block`, the `blockCode` member should be overridden instead of the `code`
  member. The `Block` itself overrides `code` to generate code for initializing variables, manage
  blocks in the IR, and so on. Thus, overriding `code` would cause the `Block` to generate incorrect
  code.

  There are two overloads of `blockCode`. One that takes a `core:asm:Block` as a parameter, and one
  that does not. The former is for cases when the current block wishes to have control of when the
  block begins (e.g. if it wishes to place some code before the block in the IR begins), and the
  latter is the normal case where the `Block` handles all initialization.

- `add`

  Adds a variable to the block. This is typically done automatically when creating instances of the
  `lang:bs:Var` class.


Many of the control structures inherit from `Block` directly, since they need to scope variables
created in the condition of an `if` statement or a `while` loop for example. Basic Storm also
provides a class that simply contains other statement. This class is called `lang:bs:ExprBlock`, and
it is the class used for ordinary blocks that appear in the Basic Storm source code. This class
simply extends the `Block` with an array that stores the expressions contained in the block. When
asked to generate code, it simply generates code for each of the expressions in turn, and any code
required to pass the result of the last expression back to the parent expression. The `ExprBlock`
also keeps track of unreachable code in the block and avoids generating dead code (e.g. after a
return statement).

The `ExprBlock` class has the following members that are relevant when developing extensions:

- `add`

  Add an expression at the end of the block.

- `insert`

  Insert an expression at a particular position. Note: this is a linear operation.


