Picture Elements
================

The presentation library provides the [stormname:presentation.Picture] element that allows creating
more complex line drawings, and similar. Its design is inspired from TikZ in LaTeX. As such, a
picture contains a set of *nodes* and a set of *edges* (actually implemented as a single set of
picture elements, [stormname:presentation.PElement]).

The picture itself acts as a regular element with regard to the remainder of the presentation. It
automatically computes a bounding box of the pictures, and centers its contents in the allocated
area. It is also possible to scale the entire contents of a picture using its `scale` attribute.

The picture element is integrated with the remainder of the presentation library. As such, a custom
syntax is available for creating picture elements conveniently. Furthermore, it is possible to apply
animations to individual elements inside a picture, just as to elements in a presentation.


Syntax
------

A picture element can be defined anywhere a regular element would otherwise be defined using the
keyword `picture`. The keyword is then followed by a block that contains the definition of the
picture. Note that a limitation in the picture syntax is that it is not possible to define
parameters for the parent layout inside the picture block. If this is necessary, wrap the picture in
a `Stack` layout or similar.

The syntax inside a `picture` block is similar to that of the layout language, but with some minor
differences. Each line corresponds to adding some element to the picture. Lines have the form:

```
[<var> =] <name> <parameters> { <attributes>; }
```

As in the layout, `<name>` is the name of a function or type, and `<parameters>` is a
comma-separated list of parameters to the function or constructor. Optionally, a number of
`<attributes>` may be specified. These have the form `<name>: <parameters>;` where `<name>` is the
name of a member in the created type, and `<parameters>` are parameters to the member. If `<name>`
refers to a member variable, and only one parameter is provided, the member variable is assigned
instead.

The optional `<var> =` allows assigning the created element to a variable in addition to adding it
to the picture. This makes it possible to reference the element later on, which is handy to position
other elements relative to the element, or to connect edges to it.

The language also contains special syntax for relative positioning. If a property is specified as
`<cardinal>: <number> of <element>`, then the library will automatically set the element's position
and alignment to achieve the desired positioning. `<cardinal>` can be any of the cardinal directions
in the layout library, `<number>` is any number, and `<element>` is either a `Node` or a `Point`. If
the element is a `Node`, then the measurement is relative the edge at `<cardinal>` to ensure that
the distance between the closest edges of the two elements two is as specified.

For example, two nodes with a connecting edge can be created as follows:

```bs
use presentation;

presentation Picture "Picture" {
    slide content "Picture" {
        picture {
            // Create a rectangle at position 0, 0 with the size of 100, 100.
            a = rectangle { minSize: 100, 100; at: 0, 0; }

            // Create a rectangle of the same size, use the relative positioning
            // to place it 200 to the east of a:s east side.
            b = rectangle { minSize: 100, 100; east: 200 of a; }

            // Connect them with an edge. Will automatically pick the proper start
            // and end points of the rectangle.
            edge a, b { toArrow: FancyArrow; }
        }
    }
}
```


As with other parts of the presentation language, all names are resolved using the normal rules for
Basic Storm. As such, it is possible to create custom elements by subclassing the relevant classes.
It is also possible to set default values by creating a function that creates an element, sets the
defaults, and returns it. For example, a rectangle with a default size can be created as follows:

```bs
Rectangle largeRectangle() {
    Rectangle r;
    r.minSize(500, 500);
    r;
}
```

This would allow using the node type `largeRectangle` in the picture syntax.


Nodes
-----

All nodes inherit from the [stormname:presentation.Node] class. A node is specified in terms of a
point (`at`), and an anchor (`anchor`). The anchor is of the type `Cardinal`, and is used to
determine what part of the node should be located at the point `at` in the picture's coordinate
system.

The size of the node is specified in terms of its minimum size (`minSize`) and the size of the
contained text (if any). The final size (as returned by `size`) is then the maximum of the `minSize`
and the size of the contained text.

There are multiple ways to access the final node's position in order to easily position other
elements relative to already created nodes. For example, `pos` can be used to get the final
rectangle that the node occupies. There are also functions that correspond to the cardinal
directions (i.e. `center`, `north`, `northEast`, `east`, `southEast`, `south`, `southWest`, `west`,
and `northWest`) that extract that particular point of the node's rectangle. Furthermore, the member
`atEdge` computes the point at the edge of the node that is at a particular angle of the node. This
is used for determining where edges start and end.

The `Node` class has the following properties:

```stormdoc
@presentation.Node
- .at(*)
- .anchor
- .margin(*)
- .minSize(*)
- .font(*)
- .style(*)
- .text(*)
- .textAnchor
- .textBrush
- .textColor(*)
- .borderBrush
- .borderColor(*)
- .fillBrush
- .fillColor(*)
```


In terms of appearance, a simple `Node` only draws the contained text. To give the node an
appearance, use any of the provided subclasses to define the geometry of the node itself. The
following nodes are available by default:

- `node`

  A plain node. Only draws the contained text.

- `rectangle`

  A node shaped like a rectangle. Draws a potentially filled rectangle with a border around it. If
  `rounded` is set to a nonzero value, then the corners are rounded.

- `circle`

  A node shaped like a circle. If `allowOval` is `true` (e.g. by calling `oval`), then the node may
  be oval.

- `imageNode <bitmap>`

  Displays an image inside the node. The property `scale` determines the size of the image. Any text
  is drawn on top of the image.


Edges
-----

An edge is a line between two points, or two nodes. The edge may optionally have arrowheads at each
end. An edge may also be bent to create a round shape if desired.

Edges are created by the `edge` function that accepts two parameters. These parameters may either be
points, or nodes. If they are nodes, the library will automatically determine the starting point
based on shape of the node. Since parameters may also be points, the behavior can be fine-tuned by
passing for example `node.east` as an explicit starting point.

An edge has the following attributes:

```stormdoc
@presentation.Edge
- .brush
- .color(*)
- .width
- .bendLeft(*)
- .bendRight(*)
- .fromArrow(Arrow)
- .toArrow(Arrow)
```

### Arrow Tips

The following arrow tips are provided:

- `PlainArrow` - an arrow that consists of two lines. The length and angle can be specified as
  parameters to customize the appearance.

- `TriangleArrow` - an arrow that consists of a filled triangle. Essentially a filled version of
  `PlainArrow`. The length and angle of the arrow can be specified as parameters.

- `FancyArrow` - a fancy, filled, arrow where the side furthest from the tip bends inward. As with
  the two previous arrows, the angle and length can be specified as parameters to the constructor.


Paths
-----

A path is a final type of elements that may be contained in a picture. They expose an interface
similar to the [stormname:ui.Path] class in the [UI
library](md:/Library_Reference/UI_Library/Rendering). They are created using the name `path` and
have the following attributes for creating the actual path:

```stormdoc
@presentation.PathElem
- .color(*)
- .at(*)
- .fill()
- .start(*)
- .line(*)
- .point(*)
- .bezier(*)
- .close(*)
```