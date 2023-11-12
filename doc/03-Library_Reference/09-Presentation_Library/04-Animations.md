Animations
==========

There are two types of animations in the presentation library: slide intro animations, and element
animations. The included ones are presented here below.


Slide Intro Animations
----------------------

Slide intro animations need to have names that end with `Intro` to be usable from the presentation
language (the presentation language automatically adds the suffix `Intro` before resolving the
name). The following ones are included in the library, but custom ones can be created as well:

- `FadeIn`

  Fades in using transparency. The duration can optionally be specified as a parameter.

- `SlideDown <fade>`

  Slide down from the top. If `<fade>` is true, then the slide is faded in as well. A custom
  duration can be specified as the first parameter.

- `Enlarge`

  Enlarge the old slide and fade it out. A custom duration can optionally be specified as a
  parameter.


### Custom Intro Animations

To create custom animations, simply create a new subclass to [stormname:presentation.SlideIntro],
and override the `draw` member.



Element Animations
------------------

Element animations need to have names that end in `Animation` to be usable from the presentation
language (the presentation language automatically adds the suffix `Animation` before resolving the
name). The following animations are provided by the library:

- `Show`

  Display the element at the specified step. No animation.

- `Hide`

  Stop showing the element at the specified stop. No animation.

- `FadeIn`

  Fades the element in from transparent.

- `FadeOut`

  Fades the element out to transparent.

- `Raise`

  Animation that fades the element in from transparent, but also raises the object while doing so.
  The height to raise the element can optionally be specified as a parameter.

- `Grow`

  Grow the element vertically.

- `Shrink`

  Shrink the element vertically.

- `ZoomIn`

  Zoom the element in. Like `Grow` but affects both dimensions.

- `ZoomOut`

  Zoom the element out. Like `Shrink` but affects both dimensions.


### Custom Element Animations

To create custom element animations, simply create a subclass to [stormname:presentation.Animation].
The subclass must have a constructor that accepts at least one `Nat`. The presentation language
passes the step to which the animation applies as the first parameter. This is expected to be
forwarded to the superclass' constructor.

The class then needs to override the following two members:

```stormdoc
@presentation.Animation
- .before(*)
- .after(*)
```

The following convenience functions are also available to simplify implementation of animations:

```stormdoc
@presentation.Animation
- .val(*)
- .smoothVal(*)
- .smooth(*)
```

Finally, there are a few convenience classes that perform some common tasks:

- [stormname:presentation.Trigger]

  ```stormdoc
  presentation.Trigger
  - .trigger(*)
  ```

- [stormname:presentation.AnimationIntro]

  ```stormdoc
  presentation.AnimationIntro
  - .before(Element, ui.Graphics, Float)
  - .after(Element, ui.Graphics)
  ```

- [stormname:presentation.AnimationOutro]

  ```stormdoc
  presentation.AnimationOutro
  - .before(Element, ui.Graphics, Float)
  - .after(Element, ui.Graphics)
  ```
