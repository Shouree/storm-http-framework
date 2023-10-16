Sizes and Offsets
=================

The intermediate language aims to be platform independent. In particular, it supports both 32- and
64-bit systems. Since the size of a pointer differs on 32- and 64-bit systems, the sizes of data
structures typically differ between the two. For this reason, the intermediate language provides two
datatypes, `core.asm.Size` and `core.asm.Offset` to make it easier to work with sizes and offsets in
a platform-agnostic manner. Both types achieve this by storing both a 32-bit and a 64-bit size and
offset respectively.


Size
----

The `core.asm.Size` type represents the size of some data in memory. As mentioned above, the `Size`
type contains two versions of the size. One for 32-bit systems, and another for 64-bit systems.

Each size consists of an unsigned integer that represents the size. In addition, each size contains
the alignment of the data. The size portion of the data is *not* aligned internally, but it is
automatically aligned when it is retrieved. The 32-bit size is retrieved through the member
`size32`, and the 64-bit size is retrieved through the member `size64`. In portable code, the member
`current` can be used to retrieve the size for the current platform.

Since the size is not aligned internally, addition and multiplication operates on the unaligned
size. It is, however, possible to force alignment of the internal size by calling the `aligned`
member. Similarly, it is possible to remove the internal alignment by calling the `unaligned`
member.

There are a number of pre-defined constants that create sizes for Storm types:

- `lang.asm.sPtr` - create a size for a pointer (4/8 bytes)
- `lang.asm.sByte` - create a size for a `Byte` (1 byte)
- `lang.asm.sInt` - create a size for a `Int` (4 bytes)
- `lang.asm.sNat` - create a size for a `Nat` (4 bytes)
- `lang.asm.sLong` - create a size for a `Long` (8 bytes)
- `lang.asm.sWord` - create a size for a `Word` (8 bytes)
- `lang.asm.sFloat` - create a size for a `Float` (4 bytes)
- `lang.asm.sDouble` - create a size for a `Double` (8 bytes)


Offset
------

The `Offset` type represents an offset into a type. An offset is signed and can thereby be negative.
Furthermore, the offset does not carry an alignment. It can, however, be manually aligned using the
`alignAs` member. The stored value can be retrieved using `v32` or `v64` for specific offsets, or
using the `current` member for the current platform.
