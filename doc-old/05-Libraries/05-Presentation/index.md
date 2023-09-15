The presentation library
=========================

Storm includes a library for creating presentations in a declarative manner. It is located in the
`presentations` package. The file `root/presentation/test/simple.bs` includes the following example
that illustrates how the library is used.

```
?Include:root/presentation/test/simple.bs?
```

The `presentation` block declares a function named `Simple` that returns a `Presentation` instance
that describes the presentation. The function is then called inside `main` to produce the instance
and show it using the `show` member. Since the `presentation` block is essentially a function, it
may contain regular Basic Storm code, but also declarative declarations of slides using the layout
DSL as described [here](md://Libraries/Layout/), as shown in the example.

The file `root/presentation/test/test.bs` contains a larger example that shows more complex layout
and also the `picture` block, which allows creation of more advanced figures in the presentation.


Live reloading
---------------

To improve the experience when creating presentations, the presentation library supports live
reloading of presentations. For this to work, the presentation library must be informed of the
location of the presentation's definition. This is done by using the `showDebug` function rather
than the `show` function to show the presentation as follows:

```
use presentation;
use lang:bs:macro;

void develop() {
    showDebug(named{Simple});
}
```

The `named{}` syntax evaluates to the `Function` object inside the compiler that represents the
presentation function itself. The presentation library then executes the function to retrieve the
presentation itself, and is able to use the `Function` object to reload the function when the user
presses the F5 key.


Exporting to PDF
-----------------

Exporting presentations to PDF is also supported using the `export` function provided in the
library. The PDF writer library in the `ui.pdf` package is used for this. This library is not
entirely complete, and may produce corrupt PDF at times, but works well in most cases. Please read
the README for that library using `help ui:pdf` in the Basic Storm interactive prompt before using
this ability.

The example above can be exported as follows:

```
use presentation;
use core:io;

void export() {
    Simple.export(cwdUrl / "export.pdf");
}
```
