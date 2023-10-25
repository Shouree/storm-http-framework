Graphics
========

The Storm standard library provides basic primitives for storing graphics in order to facilitate
interoperability between libraries, particularly libraries written in C++. As such, the facilities
provided by the standard library are quite sparse. However, the [graphics
library](md:/Library_Reference/Graphics_Library) provides facilities for loading images from
streams, and the [UI library](md:/Library_Reference/UI_Library) allows drawing them to the screen.

The graphics part of the standard library is located in the `graphics` package (i.e. not in `core`),
and contains the classes [stormname:graphics.Color] and [stormname:graphics.Image].

Color
-----

The [stormname:graphics.Color] class stores a color in RGB + alpha format. Each component is stored
as a 32-bit `Float` in the range 0-1. The alpha component is in the range from 0 (transparent) to 1
(opaque). The alpha component is *not* premultiplied in the `Color` class.

The `Color` class contains constructors that allow creating colors from color components (both with
and without alpha), both as `Floats` and as `Bytes` (range 0-255). It also contains operators `+`
and `-` for mixing colors, and `*` and `/` for scaling the color values (except alpha) with a
scalar.

There are also convenience functions for creating various colors:

- [stormname:graphics.transparent()] - Transparent color, based on the color black.
- [stormname:graphics.black()]
- [stormname:graphics.white()]
- [stormname:graphics.red()]
- [stormname:graphics.green()]
- [stormname:graphics.blue()]
- [stormname:graphics.yellow()]
- [stormname:graphics.cyan()]
- [stormname:graphics.pink()]

The [presentation library](md:/Library_Reference/Presentation_Library) contains definitions of the
DVIPS colors in the package `presentation.colors`.


Image
-----

The [stormname:graphics.Image] class stores an image in RGB + alpha format. Conceptually, the image
consists of a 2D-array of [stormname:graphics.Color]s. For increased efficiency, however, each image
is stored as a 32-bit RGBA value (i.e. 8 bits per channel). In C++, the `Image` provides access to
the underlying buffer in a format that is often efficient for other libraries to consume. It also
allows accessing individual pixels from Storm.

The interface is as follows:

```stormdoc
@graphics.Image
- .__init()
- .__init(core.geometry.Size)
- .__init(core.Nat, core.Nat)
- .get(*)
- .set(*)
- .hasAlpha()
```
