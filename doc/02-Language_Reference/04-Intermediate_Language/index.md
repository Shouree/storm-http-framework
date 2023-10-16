Intermediate Language
=====================

The Intermediate Language is an intermediate representation (IR) that is designed as a
platform-independent version of machine code that can be emitted by other languages in the system.
Storm then takes care of transforming the emitted IR into machine code that can be executed natively
by the processor in the current process. As such, the intermediate language is low-level and
generally not type-safe, even though it operates at a higher level compared to machine code or
assembly language.

Since the Intermediate Language is designed as a compilation target for other languages, it has no
formal textual representation that can be compiled by the system. It is, of course, possible to
create such a representation (even as a library), but this has not been done as it is typically more
convenient to write code i Basic Storm, or other higher level languages. Since it is not possible to
load code in the Intermediate Language, it is implemented in the package `core.asm`, rather than in
`lang`.

Since there is no formal textual representation for the IR, this part of the manual will use the
representation used when pretty-printing the IR to discuss its semantics. It will also occasionally
use [Basic Storm](md:../Basic_Storm) to illustrate how IR is emitted from other languages.


The remainder of this part of the manual consists of the following sections:

- **[Type Descriptions](md:Type_Descriptions)**

  Describes the representation of type information used in the intermediate language. To avoid
  circular dependencies between the Intermediate Language and the compiler itself during startup,
  the Intermediate Language uses a simplified representation of types compared to the rest of the
  compiler.

- **[Listings](md:Listings)**

  Describes the concepts associated with a *listing* (from *assembly listing*). Each listing is
  essentially a function in the intermediate language, and acts as the context for the instructions.
  As such, the listing keeps track of which local variables, when they are visible, and similar
  things.

- **[Operands](md:Operands)**

  Lists all operand types in the intermediate language.

- **[Instructions](md:Instructions)**

  Lists all instructions in the intermediate language, and their semantics.

- **[Usage in Basic Storm](md:Usage_in_Basic_Storm)**

  Describes how to emit code in the intermediate language from Basic Storm.

- **[Binary_Objects](md:Binary_Objects)**

  Describes how a `Listing` object can be compiled into machine code using a `Binary` object.

