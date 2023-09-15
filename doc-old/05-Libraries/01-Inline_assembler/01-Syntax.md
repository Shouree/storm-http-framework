Syntax
=======

The syntax used in the inline assembler is inspired by the Intel assembly language. This means that
each statement has the form: `<op> <dest>, <src>;`. Some statements only take one operand.

Each of the two operands can be one of:
* A register
* A literal
* A variable
* A label
* Conditionals


Registers
----------

Three different registers are available: `a`, `b` and `c`. These come in four sizes, `al`, `bl` and
`cl` for byte-sized registers, `eax`, `ebx` and `ecx` for Int-sized registers, `rax`, `rbx`, `rcx`
for Long-sized registers and `ptrA`, `ptrB` and `ptrC` for pointer-sized registers. These are
referred to by their names.

Literals
---------

Literals are numeric constants with a prefix telling their type. Size prefixes are supported: `b`
for `Byte`, `i` for `Int`, `n` for `Nat`, `l` for `Long` , `w` for `Word` or `p` for
pointer-size.

The literal 'sPtr', optionally followed by an asterisk and an integer, can be used as a
pointer-sized literal representing multiples of the pointer-size of the current plaform (4 or 8
bytes).


Variables
----------

To refer to variables in Basic Storm, simply refer to them by name. A variable will return its
representation in Basic Storm. A variable holding an `Int` can be accessed as an `Int` in assembler
and modified. Variables of object-types are pointers.


Memory access
-------------

Memory is accessed using square braces preceeded by a size specifier. The braces may contain a
pointer-sized register, optionally followed by a `+` sign and an integer literal to indicate an
offset. The size specifier for the memory access is the same as for numeric literals described
above.

Example: `i[ptrA]` reads an integer (`i`) from the address in `ptrA`.

Example: `l[ptrA + 8]` reads a long (`l`) from the address 8 bytes after `ptrA`.

Example: `p[ptrA + sPtr]` reads a pointer (`p`) from `ptrA` plus the size of one pointer (4 or 8 bytes depending on the platform).

Example: `p[ptrA + sPtr*2]` reads a pointer (`p`) from `ptrA` plus the size of two pointers (4 or 8 bytes eac depending on the platform).


Labels
--------

Labels can be placed before or after any statement, and are formed by a name followed by a colon
(`:`). Labels are referred to as `@<name>`.


Conditionals
-------------

Some op-codes take a conditional as one of their parameters. A conditional represents the result of
the last `cmp` operation. These conditionals can be one of the following:
* `ifEqual` - a equals b
* `ifNotEqual` - a does not equal b
* `ifAbove` - a is above b (unsigned)
* `ifAboveEqual` - a is above or equal to b (unsigned)
* `ifBelow` - a is below b (unsigned)
* `ifBelowEqual` - a is below or equal to b (unsigned)
* `ifGreater` - a is greater than b (signed)
* `ifGreaterEqual` - a is greater than or equal to b (signed)
* `ifLess` - a is less than b (signed)
* `ifLessEqual` - a is less than or equal to b (signed)
