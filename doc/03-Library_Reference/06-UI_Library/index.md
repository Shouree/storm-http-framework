UI Library
==========

The UI library (in the package `ui`) allows creating graphical applications in Storm. The library
integrates with the [layout library](md:/Library_Reference/Layout_Library) to allow convenient
specification of layout. It also provides hardware-accelerated 2D rendering to applications.

The library aims to provide a native look-and-feel while remaining cross-platform. As such, it uses
the native Win32 API on Windows, and Gtk on Linux.

The library can be thought of as consisting of three parts:

- [Window management](md:Windows)

- [Domain specific language for window layout](md:Layout)

- [2D rendering](md:Rendering)



Finally, a note on DPI awareness. Modern systems have the ability to specify a custom DPI value to
increase or decrease the size of elements on the screen. The UI library in Storm takes this into
account by internally scaling all measurements. As such, values visible to the application do not
always represent pixels, but rather pixels in a 100% scale (96 DPI on Windows). This means that
applications that use the UI library generally do not have to worry about the current DPI. There
are, however, some implications of this decision. Perhaps the most relevant one is related to
bitmaps. Namely, a 100% scale of a bitmap does not necessarily mean it will be displayed pixel by
pixel on the screen. Similarly, a line with a width of one, does not necessarily occupy exactly one
pixel on the screen.

On Linux, the UI library relies on the support present in Gtk. As such, setting the `GDK_SCALE`
environment variable behaves as expected. Storm does not, however, fully support the `GDK_DPI_SCALE`
variable since that only affects font sizes. Widgets will respect this variable, but not custom
rendering code. Windows supports fractional DPI scaling natively, so DPI scaling works as expected
there.


