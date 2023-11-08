Code Generation
===============

The compiler library contains the functionality required to emit code in the [intermediate
language](md:/Language_Reference/Intermediate_Language), compile it into machine code for the
current platform, and to later execute the code (usually by binding it to a `Function`). The
functionality related to the intermediate language is covered in detail in the [language
reference](md:/Language_Reference/Intermediate_Language). This section covers the types in more
detail.

In addition to the code generation facilities for the intermediate language, the library also
contains a few utilities that can be used to make code generation more convenient.


Operands
--------

The value [stormname:core.asm.Operand] represents an operand for an instruction in the intermediate
language. The different types of operands are described in detail
[here](md:/Language_Reference/Intermediate_Language/Operands).

The `Operand` class has a number of cast constructors that allow implicit conversion from registers
([stormname:core.asm.Reg]), variables ([stormname:core.asm.Var]), labels
([stormname:core.asm.Label]), [references](md:Language_Reference/Intermediate_Language/References)
([stormname:core.asm.Ref]), and source positions ([stormname:core.lang.SrcPos]). There are also a
number of functions that create operands from various types of literals.

The `Operand` type has the usual operators for comparison, and `toS` for creating a string
representation. The empty operand (created by the default constructor) can also be checked for using
the `empty` and `any` functions as usual. It also has the following operands for accessing the
contents of the operand:

```stormdoc
@core.asm.Operand
- .type()
- .size()
- .readable()
- .writable()
```

The depending on the value returned from `type`, it is possible to extract the contents of the
operand by calling the appropriate members below. These functions check the type internally and
throws an error (`InvalidValue`) if they are used incorrectly.

```stormdoc
@core.asm.Operand
- .constant()
- .reg()
- .var()
- .offset()
- .condFlag()
- .block()
- .label()
- .ref()
- .srcPos()
- .obj()
- .tObj()
```

Instruction
-----------

The [stormname:core.asm.Instr] class represents a single instruction in the intermediate
representation. The instruction format is described in detail
[here](md:/Language_Reference/Intermediate_Language/Instructions). In short, each instruction has
two operands. The first one may be written to, and the second is only ever read from. The `Instr`
class is created by one of many free functions with the same name as the op-codes described in the
language reference. These functions perform basic checking of operand sizes and properties, and
throw an error if they are used incorrectly.

The class itself has only a few members as follows:

```stormdoc
@core.asm.Instr
- .op()
- .src()
- .dest()
- .mode()
- .size()
- .alterSrc(*)
- .alterDest(*)
- .alter(*)
```

For function class, the derived class [stormname:core.asm.TypeInstr] is used. This adds the members
`type` and `member`. The `type` member contains a type description of the parameter or return type.
The `member` member is only used for function call instructions and determines whether the called
function is a member function or not.


Arena
-----

An [stormname:core.asm.Arena] describes the platform-specific bits of code generation. The function
[stormname:full:core.asm.arena()] can be used to create an `Arena` for the current platform.
Creating an `Arena` for another platform allows generating code for another platform. This is,
however, not very useful currently, as it is not possible to save generated code to disk. It does,
however, open up for future cross-platform builds.


Listing
-------

The [stormname:core.asm.Listing] object represents a sequence of instructions, alongside information
about local variables, labels, and blocks. The concepts behind the listing is described in more
detail [here](md:/Language_Reference/Intermediate_Language/Listings). In short, the listing can be
seen as representing a *namespace* for labels, blocks, and variables. As such, it is able to
generate new names for these concepts that are only usable in that particular listing. These names
are represented as instances of the types [stormname:core.asm.Label], [stormname:core.asm.Block],
and [stormname:core.asm.Var] respectively. These created names can then be used as operands to
instructions that are then added to the listing. Labels can also be added directly to the listing to
allow other instructions to reference other instructions easily (e.g. for jump instructions).

### Creation

A listing stores the expected return type of the function it will be bound to. As such, it can be
created in any of the following ways:

- [stormname:core.asm.Listing.__init()]

  Create a listing for a function that returns `void`.

- [stormname:core.asm.Listing.__init(core.Bool, core.asm.TypeDesc)]

  Create a listing for a function that may be a member function (based on `member`), and returns the
  type `result`.

There are also two overloads that accept an `arena`. This is to allow the listing to format
backend-specific things properly. It does not otherwise affect how the `Listing` works.


### Adding and Inspecting Instructions

Instructions and labels are added using the `<<` operator. The `Listing` also provides the usual
`empty`, `any`, `count`, and `[]` members to allow iterating through the instructions in the
listing. Labels that appear before a particular instruction are retrieved using the `labels`
function.


### Labels

Labels are created using the function [stormname:core.asm.Listing.label()]. It returns a new, unique
label that is valid in the listing. As mentioned above, labels can be used as operands to
instructions, and they can be inserted in between instructions. Each label is assumed to be added to
the listing exactly once.


### Variables and Blocks

Variables in a `Listing` are arranged in a tree of blocks, similarly to many block-scoped languages.
The `Listing` provides the following members to create and manipulate blocks:

```stormdoc
@core.asm.Listing
- .root()
- .createBlock(*)
- .parent(core.asm.Block)
- .isParent(*)
- .allBlocks()
- .addCatch(*)
- .catchInfo(*)
```

Variables can then be added to blocks using any of the `createVar` overloads. Parameters are treated
specially, and are created by the `createParam` overloads. Variables and parameters can then be
inspected and manipulated using any of the following functions:

```stormdoc
@core.asm.Listing
- .accessible(*)
- .isParam(*)
- .paramDesc(*)
- .varInfo(*)
- .parent(core.asm.Var)
- .prev(core.asm.Var)
- .allVars(*)
- .allParams()
- .moveParam(*)
- .moveFirst(*)
```


ExprResult
----------

The value [stormname:core.lang.ExprResult] is a utility class that can be convenient to use for code
generation in various languages. It is, for example, used in Basic Storm.

The `ExprResult` is intended to represent a result from evaluating an expression. It encapsulates
the idea that an expression may either result in some value (represented as a
[stormname:core.lang.Value]), or it may instead stop execution before it evaluates to a value (e.g.
returning from the function or throwing an exception). As such, an `ExprResult` thus has a separate
representation of *will never return*, which is distinct from *returns, but produces no value*.

It provides the following members:

```stormdoc
@core.lang.ExprResult
- .__init(*)
- .type()
- .value()
- .empty()
- .nothing()
- .asRef(*)
```

The free function `noReturn` creates the special value that indicates that the expression will not
return.


CodeGen
-------

The utility class [stormname:core.lang.CodeGen] encapsulates a [stormname:core.asm.Listing] (stored
in the member `l`) that is being used to output instructions to. It also contains additional state
that is typically needed during code generation. This state is a [stormname:core.lang.RunOn] that
indicates the current thread, and a [stormname:core.asm.Block] that represents the current block.
This means that instead of passing three variables to code generation functions, one can instead use
a single `CodeGen` object.

In addition to bundling three concepts together, it provides a few convenience functions:

```stormdoc
@core.lang.CodeGen
- .child(*)
- .createParam(*)
- .createVar(*)
```

The `createVar` functions returns a [stormname:core.lang.VarInfo] object. This object stores a
[stormname:core.asm.Var] alongside a value that records if this variable needs to be activated after
it has been constructed. It is typically not necessary to inspect this value manually, it is enough
to call the [stormname:core.lang.VarInfo.created(core.lang.CodeGen)] member as soon as the variable
has been initialized.


There are also a number of free functions that accomplish some common tasks:

```stormdoc
@core.lang
- .allocObject(*)
- .spillRegisters(*)
- .createFnCall(*)
```


CodeResult
----------

The utility class [stormname:core.lang.CodeResult] is a class that can be used by language
implementations to communicate the location of data during code generation. To illustrate the usage,
imagine you wish to generate code for a particular AST node. The node procudes a result of type `T`.
You wish to use the result eventually, but it is not important where it is located. This wish can be
expressed as a `CodeResult` and passed to the code generation function of the AST node. The AST node
inspects the wish, and updates the `CodeResult` with the location of the result. The caller can then
inspect the location and use it as desired.

The benefit of this approach is that it is in most cases not important where the result from an AST
node is located. So if the AST node represents a local variable, the AST node may just inform the
caller that the result is located in a variable already, and thus avoids a copy if possible.


A `CodeResult` can represent three different types of *wishes*, as indicated by the constructors:

- [stormname:core.lang.CodeResult.__init()]

  The caller is not interested in the result. Only evaluate the AST node for side-effects is
  applicable, and skip creating the result if suitable.

- [stormname:core.lang.CodeResult.__init(core.lang.Value, core.asm.Var)]

  The caller asks for a result of type `type`, and it needs to be stored in the provided variable.
  This means that the AST node is not able to propose another location in order to avoid copies.

- [stormname:core.lang.CodeResult.__init(core.lang.Value, core.asm.Block)]

  The caller asks for a result of type `type`. The variable needs to be visible in `block`, but
  apart from that it does not matter which variable is used. As such, this allows the AST node to
  propose an already existing variable to avoid copying.


The AST node then inspects the `CodeResult` instance to find out which type was asked for, and may
propose a location using the following functions:

```stormdoc
@core.lang.CodeResult
- .type()
- .needed()
- .suggest(*)
- .location(*)
- .created(*)
- .safeLocation(*)
- .split(*)
```

Finally, the caller may call `location` to retrieve the created value.
