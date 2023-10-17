Operands
========

An operand is some data that an instruction operates on. As such, they are similar to parameters to
functions in a higher level language. Each operand has a size associated with it. The size of an
operand affects the selection of instructions issued to the CPU. In most cases, the Intermediate
Language will verify that all operands to the same instruction have the same size. The Intermediate
Language supports four sizes of operands in general: bytes (1 byte), integers (4 bytes), longs (8
bytes), and pointers (4 or 8 bytes, depending on the platform). Note that the pointer size is
considered to be a type distinct from both integers and words, since the size of a pointer may vary
across platforms.

In the string representations, operands are always prefixed by their size. The following prefixes
are used:

- `p` - pointer size (4 or 8 bytes)
- `b` - byte sized (1 byte)
- `i` - integer sized (4 bytes)
- `l` - long sized (8 bytes)
- `(<size>)` - other size (e.g. variables)

Operands are represented by the `core.asm.Operand` type. It can store the the types of operands
described in the remainder of this page.

In the remainder of this page, we will use the following conventions:

- `<type>` is used to refer to any one of the integer- or floating point types in Storm. That is,
  `<type>` can be replaced with any one of `byte`, `int`, `nat`, `long`, `word`, `float`, or
  `double`. It can also be replaced by `ptr` to denote a pointer-sized type. This placeholder is
  typically used where it is important to differentiate between signed and unsigned versions of the
  differently sized types.

- `<size>` is used to refer to any one of the register sizes supported by the language. That is,
  `<size>` can be any one of `byte`, `int`, `long`, `float`, `double`, or `ptr`. This placeholder is
  typically used when it is irrelevant whether or not the data is signed, as it simply denotes the
  size of the data.

Constants (Literals)
--------------------

A constant is a literal value that is used by an instruction. There are two types of constants. The
simplest type has the same value for all platforms. These are created by functions named
`<type>Const`. For example, `intConst(5)` creates the integer constant `i5`. Additionally,
`ptrConst(Nat)` can be used to create a pointer-sized constant from a `Nat`. To dynamically select
the size of the constant, the `xConst` function can be used.

The second type is called a *dual constant*. It is a constant that has two separate values, one that
is used on 32-bit systems and another that is used on 64-bit systems. This is useful to store `Size`
and `Offsets` that may be different on different platforms. For this, the overloads of `intConst`,
`natConst`, and `ptrConst` may be used.

It is also possible to store pointers to objects as constants in the code. Such constants are
created with the `objPtr` function.


References
----------

A [lightweight reference](md:References) can be implicitly converted into an operand. Since
references are pointer sized by default, the operands are treated as a pointer-sized constant.

References are represented as `@<title>`, where `<title>` is what the `title` member of the
associated `RefSource` returns.


Registers
---------

The Intermediate Language provides three registers that are available for general purpose usage.
These are called `a`, `b`, and `c`. By convention, the register `a` is used to store the return
value from functions. Since the calling convention is typically handled automatically by the
Intermediate Representation, the only implication of this fact is that the system is able to
generate slightly more efficient code if the return value is already located in the `a` register.

As is the case in many assembler dialects, each register has multiple names so that it is possible
to refer to different sizes of the same register easily. The Intermediate Language uses a naming
convention similar to the Intel CPU:s. Byte-sized registers are named `<r>l` (the *low* byte of
register `<r>`), integer sized registers are named `e<r>x`, and long sized registers are named
`r<r>x`. Pointer sized registers are named `ptr<R>`. The names are summarized below:

```inlinehtml
<table>
    <tr>
        <th>Size:</th><th>Byte</th><th>Integer</th><th>Long</th><th>Pointer</th>
    </tr>
    <tr>
        <td>Register <code>a</code></td>
        <td><center><code>al</code></center></td>
        <td><center><code>eax</code></center></td>
        <td><center><code>rax</code></center></td>
        <td><center><code>ptrA</code></center></td>
    </tr>
    <tr>
        <td>Register <code>b</code></td>
        <td><center><code>bl</code></center></td>
        <td><center><code>ebx</code></center></td>
        <td><center><code>rbx</code></center></td>
        <td><center><code>ptrB</code></center></td>
    </tr>
    <tr>
        <td>Register <code>c</code></td>
        <td><center><code>cl</code></center></td>
        <td><center><code>ecx</code></center></td>
        <td><center><code>rcx</code></center></td>
        <td><center><code>ptrC</code></center></td>
    </tr>
</table>
```

The above-mentioned names are available as constants in Basic Storm.

Note: it is generally not possible to make assumptions about the contents of the parts of a register
that is not written to. For example, if an instruction writes to `al`, the lowest byte of register
`a`, then it is not possible to assume anything about the first three bytes of register `eax`, for
example. Different architectures behave differently. For example, on 32-bit X86 systems, the old
contents of the top 3 bytes remains unchanged, while they are zeroed on ARM. As such, use a suitable
cast when converting to a wider type. Casts in the other direction is, however, well-defined. That
is, writing to `eax` and then reading from `al` will yield the low byte of the value stored to
`eax`.

There are also two special-purpose registers available in the Intermediate Representation:
`ptrStack` and `ptrFrame`. The system uses them to refer to the current top of the stack
(`ptrStack`), and the start of the current stack frame (`ptrFrame`). These are used internally to
access local variables. While it is possible to modify them, it is not a good idea as it will make
variable access and exception handling to break.

The Intermediate Language does support many more registers in order to successively lower code
from platform independent code into platform dependent code. Using registers other than the ones
mentioned above will, however, produce code that is not platform independent.

Registers are represented by their name. Registers are the only operands that are *not* prefixed by
`b`, `i`, `l`, or `p` as is the case for all other operands.


Variables
---------

Variables can be used as an operand. The `Operand` type contains a type-cast constructor that
automatically converts variables into corresponding operands. Manual type-conversions are thus
typically not necessary.

In some cases, it is useful to refer to a particular part inside a variable. This can be achieved
using any of the `<size>Rel` overloads. This overload creates an operand that refers to a chunk of
the variable that is `<size>` bytes large, and starts at the supplied offset. There is also an
overload `xRel` for supplying the size as a parameter. It is worth noting that the supplied offset
needs to be aligned to the natural alignment of the underlying type to ensure that it can be
accessed properly on all platforms (e.g. ARM does not always support unaligned memory access).

Variables are represented as `[Var<id>]`, or `[Var<id>+<offset>]` in the printed representation.

Blocks
------

As with variables, blocks can be implicitly converted into an operand. Blocks are only used as
operands to the instructions that deal with blocks. It is not possible to otherwise access or
manipulate blocks in the generated code, as the concept of a block only exists at compile-time.

Blocks are represented as `[Block<id>]` in the printed representation.

Labels
------

As with variables and blocks, labels can also be implicitly converted to an operand. Labels are
typically only used with jump instructions to direct program flow. The `Operand` does, however,
support some more advanced features, such as offsets from labels, to simplify the implementation of
code generation backends. These features are, however, not universally supported.

Labels are represented as `[Label<id>]` in the printed representation.

Pointers
--------

Operands may also refer to memory by using the value in a pointer-sized register as an address. The
address may also optionally be offset by a constant offset. This type of operands are created by the
`<size>Rel` overloads that accept a register as their first parameter. For example, accessing the 4
bytes that `ptrA` points to can be done by the operand `intRel(ptrA)`. The next four bytes can be
accessed by `intRel(ptrA, Offset(sInt))`. There is also a function `xRel` that accepts a `Size`
instance as its first parameter that allows selecting the size dynamically.

As with offsets into variables, it is important to ensure that the final address is properly
aligned. Otherwise, some systems will be unable to perform the memory access.

Pointers are represented as `[<reg>+<offset>]` in the output, where `<reg>` is the name of the
register, and `<offset>` is the offset from the pointer.


Condition Flags
---------------

Finally, an operand can contain a condition flag (`core.asm.CondFlag`). The condition flag is only
used as an operand to conditional operations to determine what to check for. The `CondFlag` enum
contains the following comparisions:

### Generic comparisons

- `ifAlways`: Always true.

- `ifNever`: Never true.

- `ifOverflow`: True if overflow.

- `ifNoOverflow`: True if not overflow.

- `ifEqual`: True if equal.

- `ifNotEqual`: True if not equal.

### Unsigned comparisons

These comparisons treat the compared numbers as unsigned integers.

- `ifBelow`: True if left hand side below right hand side.

- `ifBelowEqual`: True if left hand side below or equal to right hand side.

- `ifAboveEqual`: True if left hand side above or equal to right hand side.

- `ifAbove`: True if left hand side above right hand side.

### Signed comparisons

These comparisons treat the compared numbers as signed integers.

- `ifLess`: True if left hand side less than right hand side.

- `ifLessEqual`: True if left hand side less than or equal to right hand side.

- `ifGreaterEqual`: True if left hand side greater than or equal to right hand side.

- `ifGreater`: True if left hand side greater than right hand side.

### Floating point comparisons

- `ifFBelow`: True if left hand side below right hand side.

- `ifFBelowEqual`: True if left hand side below or equal to right hand side.

- `ifFAboveEqual`: True if left hand side above or equal to right hand side.

- `ifFAbove`: True if left hand side above right hand side.


Source Position
---------------

It is also possible to store a `SrcPos` object inside an operand. This is used to emit the
`location` pseudo-operation to allow including metadata for connecting the generated machine code to
the corresponding source code.
