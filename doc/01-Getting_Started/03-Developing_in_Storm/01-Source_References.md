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
it is possible to jump to the correct character position by typing `M-g M-g` (i.e. holding down Alt
and pressing `g` twice). Emacs then asks which character to go to. Type `70` and press Enter. This
causes the cursor to go to the `+` character in the file. Other editors work differently, but are
likely to provide similar functionality.

To fix the error, we need to convert `i` to a string by adding `.toS`. That is, the print statement
should read `print("i = " + i.toS);`.


Usage with Emacs
----------------

To make it easier to go to the location of error messages in Storm, it is possible to teach Emacs
about the format of errors in Storm, and use the built-in functionality to jump between errors. To
do this, load the file `storm-mode.el` that is provided in the Storm release (or as
`Plugin/emacs.el` in the source repository). If you just want to test the functionality once, you
can open the file in Emacs, then type `M-x` followed by `eval-buffer` and then Enter. If you wish to
load it permanently, you can instead add the following line to the end of the file `~/.emacs` and
then restart Emacs:

```
(load "~/path/to/storm/storm-mode.el")
```

This adds functionality to the `compile` command in Emacs, so that it understands the format of
error messages from Storm. To use it, press `M-x`, type `compile` and press Enter. Emacs will then
ask you what compilation command you wish to execute. Replace the default with for example `storm
demo.bs` and press Enter. This opens a new buffer where the command `storm demo.bs` is executed. If
you use the unmodified version of the program above, the error above will appear. Since Emacs knows
how to interpret the error, it is now underlined and in a different color. This means that we can
jump to the error by pressing `M-x`, typing `next-error`, and finally pressing Enter. This opens up
the buffer if it is not already open, places the cursor at the error location, and briefly
highlights the characters that contain the error (the `+` in this case). If there are multiple
errors, it is possible to repeat the process to jump onwards. After fixing the error, use `M-x
compile` again. Note that Emacs remembers the last command, so it is enough to press Enter twice.

To make the commands above easier to use, it is a good idea to bind them to keys that you find
convenient. For example, the following code can be added to the end of `~/.emacs` to use `M-p` to
compile and `M-n` to jump to the next error:

```
(global-set-key (kbd "M-p") 'compile)
(global-set-key (kbd "M-n") 'next-error)
```

