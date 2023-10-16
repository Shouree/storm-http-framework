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


The intermediate language supports the following operand types:

- **Constants** (Literals)

  A constant is a literal value that is used by an instruction. There are two types of constants.
  The simplest type has the same value for all platforms. These are created by functions named
  `<type>Const`, where `<type>` is the name of any of the Storm integer and floating point types.
  For example, `intConst(5)` creates the integer constant `i5`. Additionally, `ptrConst(Nat)` can be
  used to create a pointer-sized constant from a `Nat`. To dynamically select the size of the
  constant, the `xConst` function can be used.

  The second type is called a *dual constant*. It is a constant that has two separate values, one
  that is used on 32-bit systems and another that is used on 64-bit systems. This is useful to store
  `Size` and `Offsets` that may be different on different platforms. For this, the overloads of
  `intConst`, `natConst`, and `ptrConst` may be used.

- **Registers**

  Registers...