Type Descriptions
=================

The intermediate language uses its own representation of type information. This representation is
much simpler than the type information available in Storm in general, and contains the minimal
amount of information required to follow the calling convention on the system where Storm runs. The
type information in the intermediate language therefore only contains information of the memory
layout of types, and whether any copy-constructors need to be called to copy types. It completely
lacks information about the names of members, if the type contains any functions, etc.

Due to the simplicity of the representation, this representation is called *type descriptions*. A
type description is represented by an instance of the actor `core.asm.TypeDesc`. It is possible to
acquire a `TypeDesc` from a regular type in Storm (`core.lang.Type`) by calling the `typeDesc`
member.

There are three subclasses of the `TypeDesc` class that corresponds to three categories of types
that the system needs to be aware of:

- `core.asm.PrimitiveDesc`

  This class represents primitive types. In the context of the intermediate language, a primitive
  type is a type that the CPU can handle natively, and that fits in a register. For example,
  integers, floating point numbers, and pointers.

- `core.asm.SimpleDesc`

  This class represents simple types. That is, types that the CPU may not be able to manipulate
  natively (e.g. it may be too large to fit in a register), but since it consists of a number of
  primitive types, it is possible to decompose it into one or more registers.

  Since it is possible to decompose a simple type into registers, simple types are not allowed to
  have copy-constructors or destructors.

- `core.asm.ComplexDesc`

  This class represents complex types. These types have a representation that can not be stored in
  CPU registers, and must always reside in memory. They typically also have copy constructors and/or
  destructors that need to be called at appropriate times.

  It is worth noting that it is technically possible to store most types in registers in the CPU.
  However, since a set of constructors and destructors may track the objects' location in memory, it
  can not be stored in registers since registers do not have an address.
