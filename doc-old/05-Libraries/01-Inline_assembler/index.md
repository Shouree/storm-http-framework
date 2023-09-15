Inline assembler
================

C and C++ allows adding assembler inside functions, so that some very low-level operations can be
done. This is sometimes used for optimizations, but sometimes also to do things that C and C++ can
not do otherwise. Storm also provides this functionality, but instead of being a core part of the
compiler, it is simply implemented as a library.

The inline assembler is located in `lang:asm`, and is therefore implemented as any other language in
the compiler. This means that `.asm`-files can be created and used directly with the assembler (not
implemented yet). The assembler language also works as a language extension and can therefore be
embedded inside other languages.

Separate files
---------------

This is not supported yet. Creating `.asm`-files will result in the compiler issuing a warning about
a missing reader class for `asm`.


Inline
-------

To use the assembler inside of Basic Storm, simply `use lang:asm`, and you can use the `asm{}` block
to write inline assembly code. The `asm` block does not return anything, and when evaluated it will
simply execute the instructions written inside the block. Any local variables within the current
scope are visible inside the `asm` block as well, and can be accessed as if they were regular
registers.

The following function uses inline assembler to add two integer variables:

```
use lang:asm;

Int asmAdd(Int a, Int b) {
    Int r;
    asm {
        mov eax, a;
        add eax, b;
        mov r, eax;
    }
    r;
}
```

Semantics
----------

The assembler does not generate machine code directly, instead it generates an intermediate
representation that is similar to bytecode in other languages. This bytecode is heavily inspired
from the X86 instruction set, but has some differences which aims to make the bytecode
portable. Bytecode is never interpreted, it is always translated into machine code before it is
executed.

In the bytecode, there are four different data sizes, 1 byte (`Byte`), 4 bytes (`Int`), 8 bytes
(`Long`) and pointer-size. Trying to mix different data types in the same instruction will fail,
except for some instructions (for example when taking the address of something or casting). Function
calls also have a special form.

It is worth to notice that some of the limitations in X86 assembly does not exist in the
bytecode. For example, it is not possible to have both a source and a destination operand in memory
in raw X86 assembly, but the code generation solves this by emitting extra instructions as
necessary. The code generation always takes care of preserving any registers when doing these kind
of things, which means that it is entirely transparent.
