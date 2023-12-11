Source References
=================

Storm is a bit special regarding references to source code. Instead of using line numbers like most
other languages, Storm works with character offsets in text files. This lets Storm point to
particular characters using only a single number, rather than using separate lines and columns. This
works fine when using Storm alongside an editor that understands these source references, but might
be a bit cumbersome in other situations.

To illustrate this, consider a file `demo.bs` that contains the following code:

```bs
void main() {
    for (Int i = 0; i < 10; i++) {
        print("i = " + i);
    }
}
```

When we run this file using the command:

```
storm demo.bs
```

We get the following error message (the path will of course be different on your system):

`@/home/user/storm/demo.bs(70-71): Syntax error: Can not find an implementation of the operator + for core.Str, core.Int&.`

The error starts with a source reference as indicated by the `@` character. Directly following the
`@` is the path of the file that contained the error. The two numbers in parentheses is a range of
characters that contain the problem. After that is a colon (`:`) followed by the error message.

In this case, Storm tells us that there is no operator `+` for `core.Str` and `core.Int`. In this
example it is fairly easy to deduce where the error is (there is only one `+` after all).
Regardless, the error states that the error is on characters 70-71 in the file `demo.bs`. In Emacs,
it is possible to jump to the correct character position by typing `M-g M-c` (i.e. holding down Alt
and pressing `g` followed by `c` while keeping Alt pressed). Emacs then asks which character to go
to. Type `70` and press Enter. This causes the cursor to go to the character before the `+`
character in the file. The reason for the offset of 1 is that Emacs starts counting from 1, while
Storm starts counting from 0. So, to go to the correct character, we need to add 1 to the character
number. As such, doing `M-g M-c 71` then pressing return takes us to the correct character. Other
editors work differently, but are likely to provide similar functionality.

To fix the error, we need to convert `i` to a string by adding `.toS`. That is, the print statement
should read `print("i = " + i.toS);`.


Integration with Emacs
----------------------

To make it easier to jump to the location indicated by error messages, it is possible to teach Emacs
how errors are formatted by Storm. This is described in the page about [Emacs
integration](md:Emacs_Integration).
