Layout
=======

Storm includes a library for specifying layout of rectangles inside a container (perhaps Ui
components on a screen). This library is written to be generic, and as such it is generally not
usable without providing a small amount of code in order to hook it into whatever should be laid
out.

The layout library is based around the `Component` class, which represents something that should be
laid out in a container. These containers are represented by the `Layout` class, and are responsible
for computing the position of all `Components` inside of them. `Layout` instances are also
`Components`, which means that layout can be nested arbitrarily to produce the desired layout.

Each `Layout` provides two sets of properties: global properties and local properties. The global
properties are set once for each instance of the `Layout` (eg. `border`), while local properties are
specified for each child component. These properties are managed by the `Layout` using a nested
class usually named `Info`. The syntax expects that the `add` function returns a subclass of
`Layout:Info` that is associated with the newly added component. This object is expected to contain
any local properties either in the form of member variables or functions.


Layout managers
----------------

The following layout managers are currently provided. For more detailed information, look up their
documentation in the built-in documentation.

* *Border*: Adds a border around another component.
* *Grid*: Places components inside a grid that resizes automatically.


Layout DSL
-----------

The layout system provides a small language to specify layouts more conveniently. In Basic Storm, it
looks like this (we expect that `a`, `b`, and `c` are declared elsewhere):

```
var x = layout Grid {
    wrapCols: 2;
    expandCol: 0, 1;
    a {}
    b {}
    c {
        row: 3;
        col: 3;
    }
};
```

The example illustrates the overall structure: properties are specified using `<name>: <value(s)>;`,
and components using `<name> {...}`. A property specified will first be applied as a global property
for the component (such as `wrapCols` above). If no such global property exists, a local property is
tried instead (such as `row` and `col` above). All properties are assigned in order, so if any
property has side effects (such as `nextLine` inside a `Grid`), order may matter.

Components and values can be specified using any valid Basic Storm expression, but more complex
expressions have to be enclosed in parentheses (such as a + b). If a component refers to something
that does not inherit from `Component`, the grammar attempts to convert it to a component by calling
a visible global function named `component` to do the job.
