Op-codes
=========

Here follows a listing of all currently implemented op-codes for the assembler, and their meaning.

add dest, src
---------------------

Compute `dest += src`, where `dest` and `src` are operands of the same size, either
signed or unsigned.

sub dest, src
----------------------

Compute `dest -= src`, where `dest` and `src` are operands of the same size, either
signed or unsigned.

mov dest, src
--------------------

Copy one value from `src` to `dest`, i.e. `dest = src`.

cmp a, b
-------------

Compare `a` and `b`, and store information in the flags. Use a `jmp` or `setCond` to read the
flags.

jmp to
----------

Jump to another location in the code (usually a label). Jumping into or out of blocks is not supported at the moment,
as that will not properly handle creation and destruction of variables in the local scope. As scopes
are not yet implemented, this is not a problem.

jmp cond, to
------------------

As above, but only jumps if the condition is true.
