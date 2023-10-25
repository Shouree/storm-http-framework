Graphics Library
================

The external graphics library is a complement to the types in the [standard
library](md:/Library_Reference/Standard_Library/Graphics). It provides the ability to load images
from a number of common file formats, either from disk or from a stream.

The graphics library is located in the package `graphics`, and entirely implemented in a single
shared library. Since this is an external library, it does not need to be distributed alongside
applications where it is not needed.

The library simply provides the following two functions:

- [stormname:graphics.loadImage(core.io.IStream)]

  Load an image from a [stormname:core.io.IStream]. The function inspects the first few bytes of the
  image to determine its format. For this reason, the stream needs to support peeking around 10
  bytes from the current location and onwards.

  Throws an [stormname:graphics.ImageLoadError] on failure.

- [stormname:graphics.loadImage(core.io.Url)]

  Wrapper to load a file from an `Url`. Equivalent to calling `loadImage(file.read())`, but checks
  that the file exists before attempting to open it in order to generate better error messages.


The library currently supports the following file formats:

- PNG
- JPG
- BMP
- PPM - supports both ASCII and binary encoding

On Linux, the graphics library uses `libpng` for decoding PNG images, and `libjpegturbo` for JPEG
images. On Windows, the Windows Imaging API is used. BMP and PPM use custom implementations that
support the most common image types. For example, this means that compressed BMP formats will not
work.
