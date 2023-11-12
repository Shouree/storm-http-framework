Rendering
=========

The UI library also provides the ability render 2D graphics using hardware acceleration. In order to
avoid blocking the [stormname:ui.Ui] thread with reandering (and to allow smooth animations while
dragging windows in Windows), all rendering happens on the thread [stormname:ui.Render]. As such,
rendering needs to be implemented in a separate actor, which the UI library calls
[stormname:ui.Painter].

Painters
--------

To draw in a window, it is necessary to subclass a [stormname:ui.Painter] class, and override the
`render` function. The painter can then be attached to a window by calling the `painter(Painter)`
member (or assigning to the `painter` member). This causes the system to call the `render` member
whenever the window needs to be repainted. It is also possible to schedule repaints by either
returning `true` from the `render` function, or by manually calling `repaint` in either the
`Painter` or in the `Window`. The most reliable way to render animated content is to return `true`
from the `render` function.

The `Painter` class has the following members:

```stormdoc
@ui.Painter
- .render(*)
- .repaint()
- .bgColor
```

Graphics
--------

The `render` function of the `Painter` receives a [stormname:ui.Graphics] instance. This instance is
only usable inside the `render` function, and provides the ability to draw to the window. The
`Graphics` class provides a rich set of graphic operations, including drawing text, pictures,
polygons, etc. It also supports applying [2D
transforms](md:/Library_Reference/Standard_Library/Geometry) to all drawing operations, clipping,
and transparency.

To make it easier to write composable drawing operations, the `Graphics` class maintains some
internal state that can be saved and restored on a stack. This state consists of the current
transform, the current layer's opacity, the current clipping region, and the current line width.
Furthermore, these values are manipulated in relation to the previously saved state. This means that
code that manipulates the transform works properly even if it is executed in a context where a
transformation is already applied.

It is worth noting that states that apply transparency are fairly expensive, as they require
creating internal buffers and copying them around. As such, it is a good idea to minimize states
that introduce transparency. Drawing objects with semi-transparent brushes is, however, not a
problem.

The following operations modify the stack:

```stormdoc
@ui.Graphics
- .push(*)
- .pop()
- .transform(*)
- .lineWidth(*)
- .reset()
```

The following operations can be used to draw various things:

```stormdoc
@ui.Graphics
- .line(*)
- .draw(core.geometry.Rect, Brush)
- .fill(core.geometry.Rect, Brush)
- .draw(core.geometry.Rect, core.geometry.Size, Brush)
- .fill(core.geometry.Rect, core.geometry.Size, Brush)
- .oval(*)
- .fillOval(*)
- .draw(Path, Brush)
- .fill(Path, Brush)
- .draw(Text, Brush, core.geometry.Point)
- .text(Str, Font, Brush, core.geometry.Rect)
- .draw(Bitmap)
- .draw(Bitmap, core.geometry.Point)
- .draw(Bitmap, core.geometry.Point, Float)
- .draw(Bitmap, core.geometry.Rect)
- .draw(Bitmap, core.geometry.Rect, Float)
- .draw(Bitmap, core.geometry.Rect, core.geometry.Point)
- .draw(Bitmap, core.geometry.Rect, core.geometry.Point, Float)
- .draw(Bitmap, core.geometry.Rect, core.geometry.Rect)
- .draw(Bitmap, core.geometry.Rect, core.geometry.Rect, Float)
```


Resources
---------

To speed up rendering, the UI library attempts to store certain information on the GPU. As such, it
is typically necessary to create special representations of things that should be drawn. For
example, it is not possible to directly draw a [stormname:graphics.Image] to the screen. Rather, it
is necessary to create a [stormname:ui.Bitmap] object that contains the image data first. By
inspecting the `Bitmap` object, one can see that it inherits from [stormname:ui.Resource]. This is
indeed true for all objects that may need data on the GPU.

Since many resource objects require uploading data to the GPU, it is usually a good idea to create
them once, and keep them around while rendering, for example as members in the `Painter` class.
Creating them anew on each frame is wasteful, and incurs both overhead with respect to the garbage
collector, and may use the bandwidth between the CPU and the GPU excessively.

Brushes
-------

A brush encapsulates color and pattern information to use when drawing geometry. The simplest form
of brushes are solid color brushes, that simply fill geometry with a single color. It is also
possible to create brushes that fill geometry with bitmaps and gradients. In order to create a
coherent appearance through multiple drawing operations, brushes are always applied relative to (0,
0) in the current coordinate system.

All brushes inherit from the [stormname:ui.Brush] class. This class is itself not very interesting.
There are, however a number of subclasses for various tasks:

- [stormname:ui.SolidBrush]

  Represents a brush that paints in a solid color. The color is set as a parameter to the
  constructor. It also has members `color` and `opacity` that allows modifying the color and opacity
  as needed.

- [stormname:ui.BitmapBrush]

  Represents a brush that uses a bitmap to color pixels. The brush is created with a reference to a
  `Bitmap`, and optionally a `transform` that specifies how the `Bitmap` should be transformed. The
  transform can be modified using the `transform` member.

- [stormname:ui.LinearGradient]

  Represents a brush that draws a linear gradient between two points. The gradient is defined as a
  number of [stormname:ui.GradientStop] instances that specifies the colors along the way. Two
  `Point`s define the direction and location of the gradient. These points can be modified during
  the lifetime of the gradient using `start` and `end`. Similarly, the stops can be updated using
  `stops`, but this may incur a performance penalty depending on the backend.

- [stormname:ui.RadialGradient]

  Represents a brush that draws a radial gradient from a point with a specified radius. As for the
  `LinearGradient`, the colors are specified as a number of `GradientStops` that can be modified
  using the `stops` function. The `RadialGradient` can also be transformed using a generic transform
  to create oval shapes, for example.


Paths
-----

A path is a collection of line segments, or curves. Compared to drawing individual lines (using the
`line` function in `Graphics`), segments in a `Path` are joined properly, and no overdraw occurs. A
path is represented by the [stormname:ui.Path] object. It contains methods for creating the path
piece by piece. Do, however, note that it is not efficient to re-create the path each time it is
drawn. Instead, prefer to use transformations to transform the path when drawing it, or pre-compute
a number of paths that can be used.

The `Path` class has the following members:

```stormdoc
@ui.Path
- .clear()
- .start(*)
- .line(*)
- .point(*)
- .bezier(*)
- .close()
- .bound()
```


Text Objects
------------

The class [stormname:ui.Text] represents a pre-formatted piece of text in some font. As such, when
working with text it is usually a good idea to use `Text` objects to format it ahead of time to
avoid this work on every frame. Furthermore, the `Text` class supports adding text effects, such as
different colors, different fonts, underline, and italics.

A text object is created one of the following constructors:

```stormdoc
@ui.Text
- .__init(*)
```

After that, one of the following functions can be used to inspect the shape of the text:

```stormdoc
@ui.Text
- .text()
- .font()
- .layoutBorder()
- .lineInfo()
- .boundsOf(*)
```

Text effects can then be added using any of the following functions:

```stormdoc
@ui.Text
- .color(*)
- .underline(*)
- .strikeOut(*)
- .italic(*)
- .weight(*)
- .family(*)
- .scaleSize(*)
- .effect(*)
```


To allow easy creation of formatted text object, the UI library also contains a small extension to
create [stormname:ui.FormatStr] classes. These represent a string with formatting information, and
the function [stormname:full:ui.text(ui.FormatStr, ui.Font)] can be used to create a `Text` object
from it, with the formatting information applied.

Formatted text strings are prefixed by the letter `f`. In addition to regular strings they may
contain the following sequences:

- `\i{<text>}` - format `<text>` in italics
- `\b{<text>}` - format `<text>` in bold
- `\u{<text>}` - add underline to `<text>`
- `\x{<text>}` - strike out `<text>`
- `\w[<weight>]{<text>}` - apply `<weight>` to `<text>`
- `\s[<scale>]{<text>}` - scale the size of `<text>` with a factor `<scale>` (a floating point number)
- `\c[<r>, <g>, <b>]{<text>}` - apply the color in `<r>`, `<g>`, and `<b>` (all floating point numbers) to `<text>`
- `\f['<name>']{<text>}` - apply the font family `<name>` to `<text>`

For example, a formatted string literal can be written as follows:

```bsstmt:use=ui
f"Hello \i{world}!";
```


Rendering Backends
------------------

The UI library supports a number of different rendering backends for anything that is rendered using
a `Painter`. By default, the UI library aims to select a good backend for the system in use, but in
certain selections it might be needed to override this decision. The available backends depends on
the system in use.


### Windows

On Windows, the UI library uses Direct2D for all rendering. This is currently the only supported
backend. The implementation uses a single rendering context for all rendering, so `Resource`s may be
shared freely between different rendering contexts without any extra penalty.


### Linux

On Linux, there are a number of different backends available to the GUI library. It attempts to pick
a hardware accelerated backend (using OpenGL), but in cases where drivers are missing or buggy, this
might not be desirable. Furthermore, the OpenGL-based backends are not always performant when
running X11 forwarding from another system, and mixing OpenGL with plain drawing in Gtk seems to be
buggy on some window managers.

It is possible to override the backend used by setting the environment variable
`STORM_RENDER_BACKEND` to one of the following values:

- `sw` or `software`: Use software rendering. This is very useful as a failsafe if the other modes
  cause crashes for some reason.

- `gtk`: Render using the same Cairo backend used by Gtk. On X11, this is usually accelerated to
  some extent using the XRender extension if it is available. The benefit of this method is that
  resources (bitmaps) can be kept close to the X-server where they are eventually displayed to the
  screen. This mode is also fairly robust as it uses the same code paths used by Gtk, but might
  since rendering is multithreaded, it could show bugs that are not visible to regular Gtk
  applications.

- `skia` (the default): Render using OpenGL with Skia. Just as with the `gl` backend, this backend
  is hardware accelerated and gives good performance. Due to the nature of OpenGL support in Gtk,
  the GUI library needs to keep track of separate resources for each top-level window (sometimes
  even at a finer level). As such, sharing `Resource`s between different such contexts will incur a
  small performance penalty.

Note that not all of the backends are enabled by default. The `software` and `gtk` backends are
always available. The `skia` backend is available in builds from this page, but not in the Debian
package at the moment. Setting `STORM_RENDER_BACKEND` to an unsupported value will list available
backends.

When rendering in Linux, there are some things to take into consideration due to how Gtk works. As
sub-windows is a construct entirely within Gtk, it typically only creates a single window for each
top-level window in the application (with some exceptions). In particular, this means that whenever
one part of the window is redrawn (for example, when playing an animation), any overlapping
sub-windows also need to be redrawn, which could impact performance. In cases where this is an issue
(in the rare case when drawing for example a background to a set of buttons), it is possible to
force the GUI library to create "real" windows for all places that are being rendered to by setting
the environment variable `STORM_RENDER_X_WINDOW`. This means that different `Painter`s are not able
to share resources at all.

In some cases, the user level threading in Storm triggers some latent bugs in graphics drivers (at
least, the user level parts of them). These are typically only visible when using one of the OpenGL
backends. If this is an issue, or a suspected issue, it is possible to enable one or more
workarounds that make the part of the GUI library that calls OpenGL behave a bit more closely to a
"regular" program. This is done automatically by the GUI library where issues are known beforehand,
but as not all combinations of hardware and software have been tested there might be situations
that the GUI library are unaware of.

The following workarounds are available:

- `single-stack`: Make sure to execute all calls to OpenGL with the same stack regardless of which
  UThread called the rendering code in the GUI library. This workaround fixes issues where a driver
  for some reason depends on the stack used at one point being accessible in future points (e.g. by
  accidentally storing a pointer to a stack-allocated variable). This incurs a small penalty for
  each rendering call, as the system needs to switch to another stack.

Below is a list of known hardware and software that requires one or more workarounds to function
properly. Items in this list are applied automatically.

```inlinehtml
<table>
    <tr>
        <th>Hardware</th><th>Driver</th><th>Version</th><th>Workarounds applied</th>
    </tr>
    <tr>
        <td>Intel cards</td><td>IRIS (MESA)</td><td>< 20.2.5, < 20.3.1</td><td><code>single-stack</code></td>
    </tr>
</table>
```

In case some other hardware shows issues in Storm, it is possible to use the
`STORM_RENDER_WORKAROUND` environment variable to force using one or more workarounds to be
active. The variable is expected to contain a comma-separated list of workaround names if it is
present. Note that no workarounds are applied automatically whenever `STORM_RENDER_WORKAROUND` is
present, so if your hardware and software are listed above, and you don't want to enable the
workaround, you can set the environment variable to the empty string.

If you find out that you need one or more workarounds for your hardware (or indeed, that
the available workarounds do not fix the issue), please contact
[info@storm-lang.org](mailto:info@storm-lang.org) to let me know.


Notes
------

Below are some things to consider when developing applications with the UI library:

- When drawing text, do not attempt to reduce its size too much by scaling it with a
  transformation. If text is drawn in a too small size (at least vertical size, most likely also
  horizontal), the underlying text libraries do not behave very well. Pango (used on Linux) seems to
  enter an infinite loop if attempting to draw with a vertical scale factor of zero (possibly also
  very small scales). DirectWrite (used on Windows) does not enter an infinite loop, but seems to
  fail internally with a FileFormatException in similar circumstances, which causes sporadic
  performance irregularities during rendering. Limiting the scale factor to a minimum of 0.05 seems
  to work sufficiently well on both platforms, but for small font sizes this might be too small.
