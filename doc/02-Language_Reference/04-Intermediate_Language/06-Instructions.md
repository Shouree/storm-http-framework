Instructions
============

This page lists all instructions in the Intermediate Language. In general, all instructions operate
on up to two operands. There are, however, some exceptions to this as function call instructions
require some more metadata. Instructions have the form `<instr> <dst>, <src>` in general, where
`<dst>` is an operand that the instruction may write to, while `<src>` is only read from.
Instructions that only accept one operand may read or write to the single operand.

The instruction set used is based on a subset of that used in Intel processors, but it has gradually
been generalized to target ARM processors as well. There are also numerous pseudo-instructions for
handling local variables, function calls, etc. in a platform independent way.

All instructions have a function accessible in Storm with the same name as the instruction mnemonic.
These instructions can then be appended to a `Listing` using the `<<` operator. This means that it
is possible to write fairly readable code in the Intermediate Language inside of Basic Storm. A
simple example is the following function that simply adds three to the parameter and returns the
result.

```bs
use core:asm;

void main() {
    Listing l(false, intDesc);

    Var param = l.createParam(intDesc);

    // Start of the function:
    l << prolog();

    // Load the parameter into eax:
    l << mov(eax, param);

    // Add 5 to eax:
    l << add(eax, intConst(5));

    // Return and end the function.
    l << fnRet(eax);

    print(l.toS());
}
```

Instruction List
----------------

The remainder of this page lists all instructions and describes their semantics. They are listed
based on the type of operation they perform.

Unless otherwise noted, all operands must have the size of one of the supported types (that is,
bytes, integers, longs, or pointers). Furthermore, all operands to the instruction must be of the
same size. Finally, only writable operands may appear in a position that is written to. Writable
operands are registers, variables, and memory references. These constraints are checked when an
instruction is created. The Intermediate Language throws an appropriate exception if they are
violated.

Apart from these restrictions, the Intermediate Language places few restrictions on what operands
are allowed. For example, there is no restriction that certain operands may not be in memory. Such
constraints are handled by the code generation backends. Because the code is not analyzed
immediately, some errors are not reported until the code is compiled to machine code.


## Variables and Blocks

The following instructions are pseudo-instructions that denote the beginning and end of blocks in
the generated code. As such, they do not correspond to actual machine instructions. However, they
typically mean that the backend needs to generate some code to initialize and/or destroy variables.
As such, it is not safe to assume that the values of any registers are preserved across any of these
instructions. They must therefore be stored in variables or somewhere else in memory before
executing one of these instructions.

Since these instructions are pseudo-instructions, the system also assumes that they are always
executed. For example, for all instructions that occur after a `begin` instruction, the system
assumes that the `begin` instruction has been executed. It is thus not advisable to use a
conditional jump to jump into the middle of a block, or to jump out from a block. For jumping out of
a block, the instruction `jmpBlock` performs the necessary bookkeeping to achieve this.

- `prolog`

  Emits the function prolog. Also marks the beginning of the root block in the listing. As such, no
  variables or parameters are visible before the prolog instruction. This instruction is typically
  the first instruction in all functions.

- `epilog`

  Emits the function epilog. Also marks the end of the root block in the listing. As such, no
  variables are visible after the epilog.

  It is typically not necessary to emit the epilog manually. This is done automatically by the
  `fnRet` instruction, that also handles any return values properly.

- `begin <block>`

  Marks the start of `<block>`. This involves initializing any variables in the block to zero. The
  specified block must be a direct child of the block that was the topmost block in before the
  `begin` instruction.

- `end <block>`

  Marks the end of `<block>`. This involves destroying any variables in the block that needs
  destruction. The specified `<block>` must be the currently topmost block. After this instruction,
  the direct parent to `<block>` will be the topmost active block instead.

- `activate <variable>`

  Activates the exception handling of `<variable>`. Used to mark `<variable>` as being properly
  initialized from that point and onwards. This operation does not affect the contents of any
  registers, it typically does not emit any actual instructions but only affects metadata.


### Data Manipulation

These instructions manipulate data without being concerned about the type of data.

- `mov <dest>, <src>`

  Move (copy) data from `<src>` to `<dest>`.

- `lea <dest>, <src>`

  Compute the address (load effective address) of `<src>` and store it in `<dest>`. As such `<dest>`
  must always be pointer-sized, and `<src>` may be of any size.

  If `<src>` is a reference, then the instruction extracts the associated `RefSource`.

- `swap <a>, <b>`

  Swap contents of operands `<a>` and `<b>`. This is the only instruction that writes to the second
  operand.

## Integer Arithmetics

These instructions perform various arithmetic operations on integers. The Intermediate Language
assumes that signed integers are represented as two's complement. Therefore, addition, subtraction
and multiplication are the same regardless of whether or not they operate on signed numbers. The
exception is division and modulo, which have separate instructions for signed and unsigned numbers.

- `or <dest>, <src>`

  Computes `<dest> | <src>` and stores the result in `<dest>`.

- `and <dest>, <src>`

  Computes `<dest> & <src>` and stores the result in `<dest>`.

- `xor <dest>, <src>`

  Computes `<dest> ^ <src>` and stores the result in `<dest>`.

- `not <dest>`

  Computes `~<dest>` (the bitwise inverse) and stores the result in `<dest>`

- `shl <dest>, <shift>`

  Computes `<dest> << <shift>` and stores the result in `<dest>`. The behavior is not well-defined
  if `<shift>` is larger than the number of bits in `<dest>`. A CPU may then either compute `<dest>
  << (<shift> % bits_in_dest)`, or `<dest> << min(<shift>, bits_in_dest)`.

- `shr <dest>, <shift>`

  Computes a right shift, `<dest> >> <shift>`. As with `shl`, the behavior is not well-defined if
  `<shift>` is larger than the number of bits in `<dest>`. The `shr` is suitable when right-shifting
  unsigned numbers.

- `sar <dest>, <shift>`

  Computes an arithmetic right shift, `<dest> >> <shift>`. An arithmetic shift assumes that `<dest>`
  is unsigned, and duplicates the sign-bit for each step the number is shifted to the right. As with
  `shl`, the behavior is not well-defined if `<shift>` is larger than the number of bits in
  `<dest>`. The `shr` is suitable when right-shifting unsigned numbers.

- `add <dest>, <src>`

  Computes `<dest> + <src>` and stores the result in `<dest>`. Due to the use of two's complement,
  this operation works regardless of whether signed or unsigned numbers are added.

- `sub <dest>, <src>`

  Computes `<dest> - <src>` and stores the result in `<dest>`. Due to the use of two's complement,
  this operation works regardless of whether signed or unsigned numbers are added.

- `mul <dest>, <src>`

  Computes `<dest> * <src>` and stores the result in `<dest>`. Due to the use of two's complement,
  this operation works regardless of whether signed or unsigned numbers are added.

- `idiv <dest>, <src>`

  Computes `<dest> / <src>` and stores thre result in `<dest>`. Assumes that both operands are
  signed integers.

- `imod <dest>, <src>`

  Computes `<dest> % <src>` and stores thre result in `<dest>`. Assumes that both operands are
  signed integers.

- `udiv <dest>, <src>`

  Computes `<dest> / <src>` and stores thre result in `<dest>`. Assumes that both operands are
  unsigned integers.

- `umod <dest>, <src>`

  Computes `<dest> % <src>` and stores thre result in `<dest>`. Assumes that both operands are
  unsigned integers.


## Floating Point Arithmetics

The following instructions operate on floating point numbers. As such, their operand sizes must be
either 4 (float) or 8 (double). In contrast to many architectures, floating point numbers are stored
in general purpose registers or in memory. The code generation backends transform the code as needed
for the particular platform in use.

- `fadd <dest>, <src>`

  Compute `<dest> + <src>` and store the result in `<dest>`.

- `fsub <dest>, <src>`

  Compute `<dest> - <src>` and store the result in `<dest>`.

- `fneg <dest>`

  Compute `-<dest>` and store the result in `<dest>`.

- `fmul <dest>, <src>`

  Compute `<dest> * <src>` and store the result in `<dest>`.

- `fdiv <dest>, <src>`

  Compute `<dest> / <src>` and store the result in `<dest>`.


## Type Conversions

To convert between registers and memory locations of different sizes, one of the `cast` operations
can be used. As such, it is not necessary for `<dest>` and `<src>` to have the same size for these
instructions (they would be rather pointless if they were the same size).

There are not operations to convert between signed and unsigned numbers. This conversion is easily
achievable using regular bitwise operators. To cast from an unsigned integer to a signed integer, it
is sufficient to just resize the unsigned integer to a suitable size using the `ucast` instruction.
Similarly, to convert from a signed to an unsigned integer it is enough to resize the signed integer
using `icast`, and then possibly clear the sign bit using an `and` instruction if desired.

- `icast <dest>, <src>`

  Convert a signed integer from the size of `<src>` to the size of `<dest>`. The sign bit is
  extended or moved as required.

- `ucast <dest>, <src>`

  Convert an unsigned integer from the size of `<src>` to the size of `<dest>`. Any unused bits from
  `<src>` are cleared as needed before the number is copied to `<dest>`.

- `fcast <dest>, <src>`

  Convert floating point numbers from the size of `<src>` to the size of `<dest>`. That is, convert
  from float to double or vice versa.

- `fcasti <dest>, <src>`

  Convert from a floating point number in `<src>` into a signed integer with the size of `<dest>`.

- `fcastu <dest>, <src>`

  Convert from a floating point number in `<src>` into an unsigned integer with the size of `<dest>`.

- `icastf <dest>, <src>`

  Convert from a signed integer in `<src>` to a floating point number with the size of `<dest>`.

- `ucastf <dest>, <src>`

  Convert from an unsigned integer in `<src>` to a floating point number with the size of `<dest>`.


## Control Flow

These instructions modify the control flow, either conditionally or unconditionally. As mentioned
previously, it is not possible to simply use the `jmp` instruction to skip certain
pseudo-instructions, like `begin`, `end`, or `activate`.

Note: the comparison instructions below affect the flags in the CPU. No other instructions in the
Intermediate Language are defined to update the flags. However, on some platforms, some of the
instructions do modify the flags anyway. As such, to use comparison instructions safely, they need
to appear immediately before the instruction that utilizes the flags (i.e. `jmp` or `setCond`).

- `test <lhs>, <rhs>`

  Compute the bitwise AND of `<lhs>` and `<rhs>`, and set flags accordingly. The only tests that are
  defined to work after a `test` instruction are `ifAlways`, `ifNever`, `ifEqual`, and `ifNotEqual`.
  `ifEqual` is true if `<lhs> & <rhs>` produced the number zero.

- `cmp <lhs>, <rhs>`

  Compute `<lhs> - <rhs>` and set flags accordingly. Any of the flags, except for those dedicated to
  floating point numbers, can be used to interpret the result.

- `fcmp <lhs>, <rhs>`

  Compute `<lhs> - <rhs>` and set flags accordingly. Only the flags for floating point numbers are
  guaranteed to produce relevant results.

- `setCond <dest>, <cond>`

  Sets the byte `<dest>` to 1 if the condition in `<cond>` is true. Otherwise, stores 0 in `<dest>`.
  Must appear immediately after a `test`, `cmp`, or `fcmp` instruction.

- `jmp <label>, <cond>`

  Conditional jump. Jumps to `<label>` if the condition in `<cond>` is true. Must appear immediately
  after a `test`, `cmp`, or `fcmp` instruction.

- `jmp <target>`

  Unconditional jump. Jumps to `<target>`, which may be either a label, a reference, or a pointer
  sized register.

- `jmpBlock <label>, <block>`

  Unconditiona jump to `<label>`. Assumes that `<block>` is active at the point jumped to, and
  performs necessary bookkeeping before performing the jump. Due to `<block>` being specified, it is
  possible to jump out of blocks using this instruction. This is not possible using a regular `jmp`
  instruction.

## Function Calls

These pseudo-instructions are used to emit function calls in a platform-agnostic manner. These
instructions take a `TypeDesc` that specify parameter- and return types in order for them to be able
to follow the calling convention on the target platform. As we shall see, some of them seemingly
take more than two operands, due to the required metadata.

Since these are pseudo-instructions, they can not appear everywhere. They must appear inside a
block. Furthermore, the `fnParam` or `fnParamRef` instructions that specify parameters must appear
immediately before the `fnCall` or `fnCallRef` instruction. The `fnParam` instructions do not emit
any code on their own, they only store data that is later used by the `fnCall` or `fnCallRef`
instruction. It is therefore not possible to conditionally include a parameter by using a
conditional jump to skip one of the `fnParam` instructions.

Function calls may overwrite all registers. As such, all data in registers need to be saved to
memory (e.g. in a variable or through a pointer) before the function call, and restored afterwards.

- `fnParam <type>, <src>`

  Specify a function parameter to an upcoming `fnCall` or `fnCallRef` instruction. Parameters are
  specified left-to-right.

  `<type>` is a `TypeDesc` that describes the type of `<src>`.

- `fnParamRef <type>, <src>`

  Specify a function parameter to an upcoming `fnCall` or `fnCallRef` instruction. Parameters are
  specified left-to-right.

  `<src>` is a reference (a pointer) to the data, and must therefore be pointer sized. `<type>` is a
  `TypeDesc` that describes the type of the data pointed to by `<src>`.

- `fnCall <target>, <is member>`

  Perform a function call to `<target>`, which is typically a reference, with the parameters
  specified earlier. `<is member>` is a boolean that specifies if the function is a member function
  or not. The function is assumed to return `void`.

- `fnCall <target>, <is member>, <type>, <dest>`

  Perform a function call to `<target>`, which is typically a reference, with the parameters
  specified earlier. `<is member>` is a boolean that specifies if the function is a member function
  or not. The function is assumed to return `<type>` (a `TypeDesc`). The return value is stored in
  `<dest>`, which may be either a variable, a memory location, or a register.

- `fnCallRef <target>, <is member>, <type>, <dest>`

  Like `fnCall`, `fnCallRef` calls a function and handles the return value. The difference is that
  `<dest>` is assumed to be a pointer to a location where the result should be stored. As such,
  `<dest>` is not written, but instead used to read an address from.

- `fnRet`

  Perform a return from the current function. Assumes that the function returns `void` (as specified
  to the constructor).

  As a part of the return, all variables in the current block(s) are destroyed as necessary. This
  pseudo-operation does *not* affect the current block after the instruction (since control flow
  will never proceed past the `fnRet` instruction). This means that it is possible to return from
  arbitrary positions in a function, and to jump across a `fnRet` instruction with a conditional
  jump.

- `fnRet <src>`

  Perform a return from the current function, and return the value in `<src>`. The type of `<src>`
  is assumed to match the type specified passed to the constructor of the `Listing` object.

  As with `fnRet`, it is possible to used this version to return from arbitrary points in the code.

- `fnRetRef <src>`

  Perform a return from the current function, and return the value pointed to by `<src>`. The data
  pointed to by `<src>` is assumed to match the type passed to the constructor of the `Listing`
  object.

  As with `fnRet`, it is possible to used this version to return from arbitrary points in the code.


## Metadata

These pseudo-instructions provide metadata about the program. They may be used by debuggers, or in
visualizations such as [Progvis](md:/Programs/Progvis).

- `location <pos>`

  Indicates that the instructions from here and until the next `location` instruction were generated
  to implement the source code indicated by the `SrcPos` object in `<pos>`.


## Low-level Instructions

These instructions are mainly used by the code generation backends to implement function prologs,
function calls, etc. Using them directly may result in breaking variable access and/or exception
handling.

- `nop`

  Inserts a no-op instruction. This is typically not necessary to do. Some of the backends need a
  no-op to correctly determine what variables to clean up around function calls.

- `dat <op>`

  Insert data at the current location in the code. This is used by the backends to create tables of
  metadata, or to spill large constants to memory. If execution ever reaches a `dat` literal, then
  the system will likely crash.

- `align <offset>`

  Align the next instruction to the value in `<offset>` (type `Offset`).

- `alignAs <size>`

  Align the next instruction according to the alignment in the `Size` object specified as `<size>`.

- `call <operand>`

  A low-level function call. This corresponds to the `call` instruction in the CPU, and as such it
  does not handle function parameters or return values. It is mainly used to implement the `fnCall`
  instruction by the backends.

- `ret <size>`

  A low-level function return. This corresponds to the `ret` instruction in the CPU, and it does not
  handle the calling convention or cleanup of local variables correctly. It is mainly used to
  implement the `fnRet` instruction by the backends. The `<size>` parameter is used to determine
  what size of the `a` register to use when analyzing register usage.

- `push <src>`

  Push an operand to the stack. Note, many backends require that the stack pointer is not modified
  inside functions. As such, using the `push` instruction likely breaks exception handling. Not all
  platforms support the `push` instruction.

- `pop <dest>`

  Pop an operand from the stack. Note, many backends require that the stack pointer is not modified
  inside functions. As such, using the `pop` instruction likely breaks exception handling. Not all
  platforms support the `pop` instruction.

- `pushFlags`, `popFlags`

  Push and pop the flags register to/from the stack. Note, many backends require that the stack
  pointer is not modified inside functions. As such, using these instructions likely breaks
  exception handling. Not all platforms support these instructions.

- `preserve <dest>, <register>`

  Emit metadata to record that register `<register>` has been preserved on the stack at the location
  indicated by `<dest>`. If `<dest>` is omitted, the register has been pushed on the stack. This
  pseudo-instruction only generates metadata used for exception handling.
