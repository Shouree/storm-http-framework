Inline Assembler
================

This library is a syntax extension to Basic Storm that allows writing parts of functions directly in
the [intermediate language](md:/Language_Reference/Intermediate_Language) provided by Storm. This
allows doing things that are not otherwise supported in Basic Storm. As such, it allows breaking all
forms of type safety if desired. It should therefore be used with care.

The inline assembler is located in the `lang.asm` package. To use the assembler inside of Basic
Storm, simply `use lang:asm`, and you can use the `asm{}` block to write inline assembly code. The
`asm` block does not return anything, and when evaluated it will simply execute the instructions
written inside the block. Any local variables within the current scope are visible inside the `asm`
block as well, and can be accessed as if they were regular registers.

The following function uses inline assembler to add two integer variables:

```bs
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

Status
------

Note that the inline assembler library is currently not complete. It only implements the following
instructions from the intermediate language:

- `add`
- `sub`
- `mov`
- `cmp`
- `jmp`
