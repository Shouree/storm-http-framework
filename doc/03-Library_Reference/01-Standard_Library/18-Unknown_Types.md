Unknown Types
=============

The standard library contains three types that are denoted as *unknown types*. These types have a
size and copy-constructors, but no operations. They are therefore generally uninteresting to utilize
from Storm, except when working with low-level primitives. They exist to make it possible to express
member variables that are useful in C++, but that are not possible to represent in Storm.
Occasionally, they also appear as parameters or return values to low-level functionality in Storm.

The types are as follows:

- [stormname:core.lang.unknown.INT]

  Represents some data with unknown contents that are the size of an `Int` in Storm.

  Used for variables marked as `UNKNOWN(INT)` in C++.

- [stormname:core.lang.unknown.PTR_GC]

  Represents some pointer data that needs to be garbage collected. This means that if the pointer
  points to memory managed by the garbage collector, it needs to point to the start of the
  allocation. Aside from that, there are no guarantees for the contents of the pointer.

  Used for variables marked as `UNKNOWN(PTR_GC)` in C++.

- [stormname:core.lang.unknown.PTR_NOGC]

  Represents some pointer-sized data that should *not* be garbage collected. Because of this, the
  data may contain anything, even integers.

  Used for variables marked as `UNKNOWN(PTR_NOGC)` in C++.
