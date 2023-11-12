Slide Layout
============

The presentation library relies on the layout library for layout. As such, all layouts from
[there](md:/Library_Reference/Layout_Library) can also be used in the presentation library. In
addition to those, the presentation library provides a number of additional layouts to make creation
of presentations easier.


Slide Styles
------------

Some of the layout provided by the library are in the form of general slide-styles. These are
intended to be used as the top-level layout of a slide (i.e. directly after the `slide` keyword),
and provide rudimentary layout:


- `title <title>, <subtitle>`

  Creates a title slide containing the title of the presentation and a subtitle (or the name of the
  author). Uses a `Grid` layout that can be further extended if desired.

- `content <title>`

  Creates a slide with a title and space for additional elements. This slide is thus intended as a
  "regular" slide in the presentation. Uses a custom layout (`CaptionLayout`) that only supports one
  child component. As such, adding more than one element to the content causes other elements to be
  discarded.

- `columns <title>`

  Like `content`, but creates a `ColumnLayout` as the body, so that it is not necessary to create
  one separately.

- `grid <title>`

  Like `content`, but creates a `Grid` as the body, so that it is not necessary to create one
  separately.


As mentioned previously, the slide styles are simply functions that create suitable elements and
layout. It is thus easy to create custom styles if desired.


Layout
------

Below is a list of the additional layout elements that the presentation library provides:

- `CaptionLayout`

  A layout for providing a caption at the top of a slide, and allocating the remaining space to a
  single element. Has the following attributes:

  ```stormdoc
  @presentation.CaptionLayout
  - .align
  - .space
  - .contentLeftMargin
  - .contentRightMargin
  ```

- `ColumnLayout`

  A layout that places elements in equally sized columns. The same effect is achievable with `Grid`,
  and this is therefore a simplified version of a `Grid`. The attribute `padding` dictates the space
  between each column.

- `RowLayout`

  A layout that places elements in rows, one after the other. The height of elements are used to
  place the elements. As such, it is useful to create a flow of different lines of elements (e.g.
  paragraphs and lists).

  It has the following attributes:

  ```stormdoc
  @presentation.RowLayout
  - .padding
  - .left
  - .right
  - .center
  ```

- `anchor <cardinal>`

  Anchors an element to the specified direction in a layout box that is larger than the minimum size
  of the element. This typically has the effect of causing the element to use its natural size
  instead of expanding to the container. Of course, the [stormname:layout.Anchor] has the same
  effect, this is just a convenience function for the layout.

- `Group`

  A `Group` makes a set of elements appear as once for the purposes of animations. As such, to
  animate multiple separate elements as a single unit, it is necessary to place them inside a group.
  Layout-wise, the `Group` does not perform any layout except allocating the same space to all its
  children. It is, of course, possible to use layout managers inside the group if desired.

- `WithBackground <color>`

  Add a background behind an element. Inherits from `Group` to allow animations.

- `WithBorder <color>`

  Add a border around an element. Inherits from `Group` to allow animations. Has the following
  members for manipulating the border:

  ```stormdoc
  @presentation.WithBorder
  - .margin(*)
  - .border(*)
  - .width(*)
  ```

- `WithCaption <text>, <cardinal>, <style>`

  Adds the caption `<text>` to an element, placing the caption at `<cardinal>` and using the style
  `<style>` to draw it. Inherits from `Group` to allow animations.
