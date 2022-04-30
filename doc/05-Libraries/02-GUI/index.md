GUI
====

Storm includes a GUI library for creating graphical applications. This library uses an additional
thread to handle UI events, and another thread to handle repaints. UI elements are based around the
`Window` class. A `Window` is a rectangle on the screen, maybe obscured by another window. Each
window, except for `Frames`, have a parent window in which they are contained. `Frames` are special,
as they are the top-level windows. These are usually decorated by the operating system with a border
so that the user can move and close the window easier.


Repaints are handled by a different thread, named `Render`. This thread is in charge of re-painting
all windows when they need to. Repaints are done either when the operating system requests a
repaint, when you call `Window.repaint()`, or if you want your window to be redrawn at each frame
rendered to the display. Actual drawing is handled by a `Painter` class, which is attached to a
window using `painter = MyPainter`. The `draw` member of the painter class will then be called
whenever the window needs to be redrawn.

DPI awareness
-------------

The GUI library is DPI aware on Windows. This means that the contents of windows might be rendered
at a different resolution than what was requested by the program according to the user's
preferences. This is handled transparently by the GUI library by transforming any coordinates
supplied accordingly. This means that programs using the Gui library may assume that coordinates and
sizes are always relative to a 96 DPI resolution (the standard size in Windows), and leave the rest
to the GUI library. This does, however, have some implications. Programs can not assume that one
"coordinate step" represents one pixel, neither when drawing nor when positioning components in a
window. Furthermore, as the physical size of fonts do not always scale linearly with the font size,
long chunks of text may sometimes appear to take more space, and sometimes less. When using
automatic layout, this is generally handled gracefully anyway.

On Linux, Storm relies on the support in Gtk+. Thus, setting `GDK_SCALE` to an integer value behaves
as expected. Storm does, however, not fully support the `GDK_DPI_SCALE` flag, as that only affect
font sizes. Currently, widgets respect this flag, but not custom drawing code.

Rendering
---------

Drawing things in the UI library is done using a Painter and the corresponding Graphics object
created by the Painter. This system is designed to use hardware acceleration if available, and
therefore most things that are possible to draw are represented by objects that cache some resources
associated with the rendering state. As such, when drawing in the GUI library, it is a good idea to
create resources up front and keep them alive as long as possible to maximize rendering performance
(and to minimize the work the GC needs to do).

This system is designed to be pluggable and to support multiple rendering backends at the same
time. For example, the package `ui.pdf` contains an example that renders to a PDF file instead of to
the screen. This is accomplished by creating a subclass to the `Graphics` class and implementing all
methods there. This class can then be passed along to the same rendering code that is used for
drawing to the screen. If the new backend needs to associate some resources with the classes that
represent drawable resources (for example `Bitmap`s), this is possible by associating the `Graphics`
object with a `GraphicsMgr` instance. The `GraphicsMgr` contains a set of functions that are called
to create and destroy such resources, and the system will then ensure that they are managed
appropriately. These objects are lazily created, and can be retrieved by calling the `forGraphics`
method on a `Resource` object.

It is worth considering that while a single `Resource` may be used in different contexts without
issue, the system is optimized around the case that a single `Resource` will only be used with a
single backend. This means that a `Resource` is good at storing a single piece of associated data,
but storing multiple pieces incurs a slight overhead. This is normally not a problem, but worth
keeping in mind when rendering to many things at the same time.

Rendering backends
------------------

The GUI library supports a number of rendering backends for anything that is rendered through a
`Painter` (i.e. not default components, such as buttons). By default, the GUI library attempts to
select a good backend for the system in use, but in certain situations the selection might need to
be modified. The available backends do depend on the system in use.

### Windows

On Windows, the GUI library uses Direct2D for all rendering. There is currently no other backend
available. In the current implementation, the backend uses a single rendering context for all
rendering, so `Resource`s may be shared freely between different rendering contexts without any
extra penalty.


### Linux

On Linux, there are a number of different backends available to the GUI library. It attempts to pick
a hardware accelerated backend (using OpenGL), but in cases where drivers are missing or buggy, this
might not be desirable. Furthermore, the OpenGL-based backends are not always performant when
running X11 forwarding from another system.

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
- `gl` (not enabled by default): Render using the (beta) OpenGL backend in Cairo. This mode provides
  significant performance improvements over both `gtk` and `sw`, especially when working with larger
  bitmaps. Due to the nature of OpenGL support in Gtk, the GUI library needs to keep track of
  separate resources for each top-level window (sometimes even at a finer level). As such, sharing
  `Resource`s between different such contexts will incur a small performance penalty.
- `skia` (the default): Render using OpenGL with Skia. Just as with the `gl` backend, this backend
  is hardware accelerated and gives good performance. In many cases, performance with Skia is better
  than the OpenGL backend in Cairo. Just as with the `gl` backend, rendering contexts in different
  top-level windows use different GL contexts, and can therefore not share rendering resources.

Note that not all of the backends are enabled by default. The `software` and `gtk` backends are
always available. To use the `gl` backend, the Gui library needs to be compiled with the `cairogl`
flag to mymake. The `skia` backend is available in builds from this page, but not in the Debian
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

| Hardware    | Driver       | Version            | Workarounds applied |
|-------------|--------------|--------------------|---------------------|
| Intel cards | IRIS (MESA)  | < 20.2.5, < 20.3.1 | `single-stack`      |

In case some other hardware shows issues in Storm, it is possible to use the
`STORM_RENDER_WORKAROUND` environment variable to force using one or more workarounds to be
active. The variable is expected to contain a comma-separated list of workaround names if it is
present. Note that no workarounds are applied automatically whenever `STORM_RENDER_WORKAROUND` is
present, so if your hardware and software are listed above, and you don't want to enable the
workaround, you can set the environment variable to the empty string.

If you find out that you need one or more workarounds for your hardware (or indeed, that
the available workarounds do not fix the issue), please contact
[info@storm-lang.org](mailto:info@storm-lang.org) to let me know.

Layout
-------

The GUI library is integrated with the [layout library](md://Libraries/Layout). The library provides
a convenient way of declaring windows with layout in Basic Storm as follows:

```
window MyWin {
    layout Grid {
        wrapCols: 2;
        expandCol: 0;
        Button a("A") {}
        Button b("B") { rowspan: 2; }
        Button c("C") {}
        nextLine;
        Button d("D") {}
        Button e("E") {}
        Button f("F") { row: 3; col: 2; }
        Button g("G") { row: 4; col: 1; colspan: 2; }
    }

    init() {
        init("My window", Size(200, 200));
        create();
    }
}
```

When using the `window` keyword instead of `class`, it is possible to specify a `layout` block
inside the class. By default these classes inherit from `ui.Frame`, but this can be overridden using
the keyword `extends` as usual.

The layout language is also extended by the ui library to allow declaring variables inside the
layout. These variables will become member variables inside the class itself, and the initialization
will be performed inside the constructor. Since the initialization will be performed inside the
constructors of the class, it is possible to use any local variables declared before the `init`
block in the constructor inside the initialization, even though this easily gets confusing.


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
