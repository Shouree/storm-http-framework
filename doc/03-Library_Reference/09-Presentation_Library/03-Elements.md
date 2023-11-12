Slide Elements
==============

The presentation library provides a number of elements that can be used to create slides:


- `Heading <text>, <style>` (or `heading <text>` or `subHeading <text>` to use defaults)

  An element that displays a single line of text, indented for headings. The helpers `heading` and
  `subHeading` creates headings that use the default style of the presentation.

- `Paragraph <text>, <style>` (or `par`)

  An element that displays a single line of text, similar to `Heading`. This component is, however,
  intended for paragraph text.

- `List <points>, <font>, <space>, <brush>` (or `list <points>` to use default style)

  Creates a list, each item is a string from the array `<points>`. The default is an unordered list
  (with bullets), but numbers and letters are also supported.

  Note: due to overload resolution, Basic Storm is not able to disambiguate the type of an empty
  list (could be either `FormatStr` or `Str`). Specifying at least one string solves the problem.

  Note: If list elements are line-wrapped, then the reported height during layout is wrong, which
  has unintended side-effects. Manually wrapping lines (with `\n`) solves this problem.

  The list has the following attributes:

  ```stormdoc
  @presentation.List
  - .bulletSize
  - .margin
  - .padding
  - .lineSpace
  - .ordered
  - .letters
  ```

- `Image <bitmap>` (or `image <bitmap>`)

  Displays a bitmap. The default behavior is to stretch the image to fill the allocated space. The
  `minSize` member does, however, return the actual size of the image, which causes the image to be
  very large in some layout managers. Setting `scale` to a proper value solves this issue. The
  `Image` element has the following attributes:

  ```stormdoc
  @presentation.Image
  - .scale(*)
  - .fit
  - .fill
  ```

- `PageNumber <style>` (or `pageNumber` to create with default style)

  A text field that contains the number of the current page. This relies on the page number being
  located in the background slide to retrieve the proper page number (otherwise, it will show the
  current animation step).

  The attribute `offset` specifies how the numbering should be offset. The default is that the first
  slide is zero.

- `BigArrow <angle>`

  Create a big arrow, aimed at `angle`.

- `HLine <color>`

  Creates a horizontal line. Useful together with `Grid` to make tables. Allows specifying the with
  using `width` and the minimum length with `minLength`.

- `VLine <color>`

  Creates a vertical line. Useful together with `Grid` to make tables. Allows specifying the with
  using `width` and the minimum length with `minLength`.

- `xBarGraph` and `yBarGraph` (in the package `presentation.graph`)

  Creates a bar graph with bars on the x or y axis. Allows customizing the look and colors of the
  graphs, and provides custom animations for the contents of the graph.



Colors
------

The package `presentation.colors` defines many DVIPS colors for use in presentations.


Custom Elements
---------------

Elements are simply classes that inherit from the [stormname:presentation.Element] class. Elements
are simply rectangles on the screen that will be rendered by the presentation system. Implementing a
custom element requires implementing the following members:

```stormdoc
@presentation.Element
- .minSize
- .draw(ui.Graphics)
```

The system also supports interactive elements. These are able to respond to mouse interactions. Such
elements must inherit from the [stormname:presentation.InteractiveElement] class, which has the
following members:

```stormdoc
@presentation.InteractiveElement
- .mouseMoved(*)
- .mouseClicked(*)
- .mouseDblClicked(*)
```