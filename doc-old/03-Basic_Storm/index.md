Basic Storm
============

Basic Storm is the general purpose language currently implemented in the Storm compiler. It is
largely based on the syntax of C and Java, but also inspired from LISP. Basic Storm aims to be close
to what is exposed to the compiler, and is therefore suitable to explore the compiler itself. It
also aims to be relatively simple, but utilizing the extensibility of Storm to provide simple syntax
for common tasks. Since Basic Storm is implemented using the parser in Storm, it also benefits from
the same extensibility that Storm provides all languages, even though Basic Storm is strictly not
implemented completely inside Storm.

The philosophy of Basic Storm is, just like LISP, that everything is an expression. In many cases,
it is safe to ignore this fact, but it opens up a lot of flexibility, especially when generating
code. One of the more outstanding differences is that blocks and if-statements are expressions, not
statements. A block returns the last expression written in it, which allows the programmer to
introduce temporary variables in the middle of an expression among other things.

Most of Basic Storm is implemented in C++, alongside the compiler. However, some parts are
implemented in Basic Storm itself. Examples of this are the array syntax (`T[]`), array
initialization (`T:[x, y, z]`) and the string concatenation syntax (`x # y`). The implementation of
these operators are present in the `lang.bs` package alongside the entire syntax of Basic Storm.

To make it easier to manipulate syntax, Basic Storm provides a set of convenient syntax extensions
for this task, present in `lang.bs.macro`. To use these, you need to include the `lang.bs.macro`
package in the relevant files. This package also contains the handy `explore{}` syntax, which
outputs information about the name that is enclosed in the brackets. This can be used to
interactively explore types and packages from the top loop. Note, that `explore`ing a package
currently does _not_ load it. To make sure that the package or type is loaded, simply try to access
something inside it first by just trying to call something. It probably will not compile, but it
still causes things to get loaded. This is a slight oversight in the compiler, and will be addressed
in the future.

Another tool that helps developing new syntax in Basic Storm is the `dump{}` syntax. Whenever the
`dump{}` syntax is encountered compile time, it will print whatever syntax object is inside the
brackets using the standard print function. This can be used to see what different syntaxes actually
represent. To see how it works, try `dump{ "A" # "B" }` or `dump{ Int:[1, 2, 3, 4] }`.

Basic Storm source code is saved in `.bs`-files. Files are read using the standard text input of
Storm, and therefore supports all formats described [here](md://Storm/Text_IO).
