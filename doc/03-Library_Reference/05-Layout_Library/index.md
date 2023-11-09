Layout Library
==============

The [stormname:layout] package contains a library for specifying the layout of a set of rectangles
(for example, UI components in a window). By itself, the library is not very useful, but it requires
a small bit of code to integrate it with another library. This is provided for the [UI
library](md:/Library_Reference/UI_Library) and the [presentation
library](md:/Library_Reference/Presentation_Library) for example.

The layout library does not introduce additional coordinate systems. That is, all coordinates
produced by the library are in the same coordinate system as those provided to the layout system.
Coordinates are never relative to the root container or similarly.

Core Concepts
-------------

The library is centered around the class [stormname:layout.Component], which represents a rectangle
on the screen. A `Component` itself does not actually store a position, but it rather provides
callbacks to retrieve and set the position. The `Component` provides the following members:

```stormdoc
@layout.Component
- .pos(*)
- .minSize()
- .findAll(*)
```

The second important class is [stormname:layout.Layout]. It represents a collection of `Component`s
that it is responsible to assign positions to. The `Layout` class extends `Component` which means
that it is possible to nest layout logic to create more complex layout rules.

The `Layout` class introduces the following members in addition to the ones in `Component`:

```stormdoc
@layout.Layout
- .space
- .layout()
- .add(*)
```

As noted above, the `add` function returns a [stormname:layout.Layout.Info] class that contains
layout-specific information about the child. The `Info` class in `Layout` is empty, but other layout
managers subclass the `Info` class in `Layout` to add custom data there. For example, the `Grid`
layout stores the row and column that the child should be placed in inside the `Info` class. This
information lets the DSL (see below) access the properties conveniently.

Cardinal Directions
-------------------

The layout library provides a set of cardinal directions to specify relative directions. These
directions are described by the [stormname:layout.Cardinal] class and subclasses that implement some
common directions. The `Cardinal` class has the following members:

```stormdoc
@layout.Cardinal
- .place(*)
- .pick(*)
- .align(*)
- .opposite()
- .direction()
```

The following cardinal directions are available:

- [stormname:layout.center]
- [stormname:layout.north]
- [stormname:layout.northEast]
- [stormname:layout.east]
- [stormname:layout.southEast]
- [stormname:layout.south]
- [stormname:layout.southWest]
- [stormname:layout.west]
- [stormname:layout.northWest]

Layout Managers
---------------

The library provides the following layout managers. Of course it is possible to create custom layout
managers by subclassing the `Layout` class.

- [stormname:layout.Border]

  A simple layout manager that adds a border around a single child within. If multiple children are
  added, all but the last one is discarded. The border size is set using the `border` member.

- [stormname:layout.Stack]

  This layout manager simply stacks components on top of each other. Essentially, it is an adapter
  that allows placing multiple components in the space where a single component would ordinarily
  fit.

- [stormname:layout.Anchor]

  This layout manager aligns its contents (a single component) to the specified cardinal direction.
  This is useful in situations where a small component should be placed in a large location, but the
  component should not be stretched. The `Anchor` layout thus uses the `minSize` of the component,
  and places it along the edge or in the center of the allocated space.

  It is also possible to modify the placement of the contained component using `xShift` and `yShift`.

- [stormname:layout.Grid]

  ```stormdoc
  layout.Grid
  ```

Finally, the library provides a single component that is useful in some cases:

- [stormname:layout.FillBox]

  A simple component that simply takes up space. Can be used in conjunction with the `Grid` layout
  for example to create extra spacing.


Integrating the Layout Library
------------------------------

To use the layout library elsewhere, it is necessary to create a custom `Component` that overloads
the `pos(Rect)` member and assigns the position to whatever should be laid out. It is also useful to
implement the `minSize` member in order to provide the layout library with information about the
minimum size of components.

When using the domain specific language, it is often inconvenient to have to create the custom
component manually. For this reason, the domain specific language will automatically call the
`component()` function on all objects that do not already inherit from `Component`. The
`component()` function is expected to return a `Component` that corresponds to the parameter type.


Domain Specific Language
------------------------

The layout library provides a small domain specific language so that layouts can be specified more
conveniently. The DSL is designed to be extended by other libraries to suit their needs. As such,
the syntax provided here is quite simple.

The syntax consists of a tree of *layout declarations*. Each such declaration has the following
form:

```
<type-or-name>(<parameters>) {
    <body>
}
```

Here the sequence `<type-or-name>(<parameters>)` is a call to a constructor, or a function. The
result of evaluating this call will be inserted in the layout. The parentheses can be omitted if no
parameters are required. As such, `<type-or-name>` may also be the name of a variable in the
surrounding context. As mentioned above, if the type specified does not inherit from `Component`,
then the language will attempt to call the `component()` function to create a component. The
function is found using the standard Basic Storm lookup rules.

The `<body>` may consist of zero or more nested *layout declarations* or *property assignments*.
Nested layout declarations are like regular layout declarations. Such objects will be added using
the `add` member of the outer layout object. As such, nesting declarations in this way assumes that
the parent declaration refers to a `Layout`.

Property assignments modify the behavior of the layout element. A property assignment has the
following form:

```
<identifier>: <parameters>;
```

or

```
<identifier>;
```

Where `<identifier>` is an identifier, and the optional `<parameters>` is a comma-separated list of
expressions. A property assignment attempts to call the function `<identifier>(<parameters>)` on the
layout element for the current block. These functions are typically designed to modify the behavior
of the element, which is why they are written in a different form from regular function calls. If no
parameters are required, the second form can be used. The same syntax can also be used to assign
values to member variables.

If `<identifier>` with suitable parameters is not found in the layout element, the language looks
for a suitable function or member in the `Info` class that was returned when the element was added
to its parent. This means that it is also possible to specify child-specific properties related to
the parent layout inside the block used for the child.

To start a layout declaration in Basic Storm, the keyword `layout` is used. The example below uses
the layout syntax to create a simple layout of a few components:


```bs
use layout;

Layout createLayout(Component a, Component b) {
    var x = layout Grid {
        wrapCols: 2;      // Executes: Grid.wrapCols(2)
        expandCol: 0, 1;  // Executes: Grid.expandCol(0, 1)

        a {}
        FillBox(1, 1) {}

        b {
            row: 3;     // Executes: Grid.Info.row = 3
            col: 3;     // Executes: Grid.Info.col = 3
        }
    };
    return x;
}
```

The code above is roughly equivalent to the following:

```bs
use layout;

Layout createLayout(Component a, Component b) {
    Grid x;
    x.wrapCols(2);
    x.expandCol(0, 1);

    x.add(a);

    x.add(FillBox(1, 1));

    var cInfo = x.add(b);
    cInfo.row = 3;
    cInfo.col = 3;

    return x;
}
```
