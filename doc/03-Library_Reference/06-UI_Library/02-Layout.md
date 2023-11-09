Layout
======

The UI library provides an integration with the [layout
library](md:/Library_Reference/Layout_Library) to allow defining layouts for windows easily. For
this purpose, the UI library provides three keywords that are used to define windows: `window`,
`frame`, and `dialog`. They work the same, except that they create classes that are derived from
`Window`, `Frame`, and `Dialog` respectively.

Inside a class defined using one of these keywords it is possible to include a `layout` block that
specifies the layout of the window. The contents of the layout block follows the [domain specific
language](md:/Library_Reference/Layout_Library) from the layout library. However, the UI library
extends the syntax to also allow declaring variables inside the layout syntax. Declared variables
will be added as members of the class, and will be initialized in the constructor. The UI library
also wraps the entire layout in a border.

For example, a frame can be declared as follows:

```bs
use ui;
use core:geometry;

// Creates an actor that inherits from ui.Frame.
frame MyFrame {
    layout Grid {
        expandCol: 0;
        Button a("A") {}
        Button b("B") { rowspan: 2; }
        nextLine;

        Button c("C") {}
        nextLine;

        Button d("D") {}
        Button e("E") {}

        Button f("F") { row: 3; col: 2; }
        Button g("G") { row: 4; col: 1; colspan: 2; }
    }

    init() {
        init("My Window", Size(200, 200));
        create();

        a.onClick = () => print("Hello!");
    }
}

void main() {
    MyFrame frame;
    frame.waitForClose();
}
```

Note that the UI library integrates computations of minimum size from the layout library. As such,
the system will not allow the window to become smaller than its contents.
