Presentations
=============

A `Presentation` consists of a sequence of `Slide`s. Each `Slide` is in turn a set of `Element`s,
that are laid out by a supplied [layout object](md:/Library_Reference/Layout_Library). Slides are
typically created using the domain specific language. As such, this page will introduce the domain
specific language along with the basics of the data representation for presentations.

This page focuses on the parts of the interface that are relevant to using the library. For details
on all classes, the reader is encouraged to read the built-in documentation of the relevant classes.


The Presentation Class
----------------------

The presentation represents an entire presentation. As mentioned above, a presentation consists of
zero or more slides. In addition to the slides, the presentation contains a number of variables that
specify general properties of the presentation. These are made available to components, so that it
is possible to set default fonts once, and have the remaining components use them automatically.

The `Presentation` class contains the following variables:

```stormdoc
@presentation.Presentation
- .title
- .size
- .topLeftBorder
- .bottomRightBorder
- .border(*)
- .headingStyle
- .contentStyle
- .headingAlign
- .animationTime
- .background(*)
```

The `TextStyle` type mentioned above is a class that specifies how text should look and be laid out.
It contains a `Font` that specifies which font to use, a `Brush` that specifies the color of the
text, and a `Float` named `space` that determines the default spacing after each text element.



Presentation Definitions
------------------------

Presentations are defined using the `presentation` keyword at top-level:

```
presentation <name> <title> {
    <contents>
}
```

The syntax above defines a function with the name `<name>` that creates a presentation with the
title `<title>` (a string literal). The title will be used as the window title of the window that
displays the presentation.

The `<contents>` of the definition may contain regular Basic Storm code. The code is executed in a
context where the presentation to be created is bound to the name `this`. This means that member-
variables and functions of the presentation object can be accessed as if they were in the current
scope.

Most importantly, the `presentation` block allows creating slides with the `slide` keyword as
follows:

```
slide <layout>
```

or

```
slide <intro> <intro-params> => <layout>
```

Where `<layout>` specifies the layout of the slide using a slight variant of the syntax from the
[layout library](md:/Library_Reference/Layout_Library). The difference is that the presentation
language introduces another syntax for specifying function- or constructor calls that do not require
parentheses. As such, it is possible to specify a heading as follows:

```
heading "My heading" {}
```

Additionally, whenever the name (e.g. `heading`) refers to a function, the presentation library also
searches for an overload that accepts a `Presentation` object as its first parameter. It is thereby
possible to create components that respect global configuration options that are present in the
`Presentation` object.

The optional `<intro>` and `<intro-params>` specify an optional intro animation for the slide. The
system searches for a call `<intro>Intro` (e.g. a class), and attaches it to the slide as an intro
animation.

Since all syntax look up names using the regular Basic Storm name resolution rules, it is possible
to add custom components simply by defining new classes or functions that implement the desired
behavior.


The system automatically inserts a border around all slides. This can be inhibited by using the
keyword `borderless slide` instead of just `slide`. Similarly, the background of a slide can be
specified using the `background` keywword instead of the `slide` keyword.

For example, a simple presentation may look like this:

```bs
use presentation;

presentation Simple "Simple presentation" {
    slide title "My presentation", "My name" {}

    slide FadeIn => content "First Slide" {
        list [ "First bullet", "Second bullet" ] {}
    }
}
```

Animations
----------

In addition to intro animations for entire slides, it is also possible to animate individual
elements. This is done by adding special properties to elements. This is only possible to do for
elements that are drawn, not for elements that only exist in the layout hierarchy. The special
element `Group` can be used to group multiple elements into one group to animate them as a unit.

The syntax for animation looks as follows:

```
@<step>: <type>
```

or in general

```
@<step>[+<time>][,<time>]: <type>
```

The `<step>` is an integer that specifies which step the animation should apply to. Step zero occurs
when the slide is first shown, at the same time as any slide intro animation. The optional part
`+<time>` that the animation shall be delayed by the specified time. The part `,<time>` specifies
the duration of the animation. Both times are expressions that evaluate to a `Duration`. Typically
they are specified as `150 ms` for example.

To illustrate how they can be used, consider the following example:

```bs
use presentation;

presentation Simple "Simple presentation" {
    slide title "My presentation", "My name" {}

    slide FadeIn => content "First Slide" {
        ColumnLayout {
            par "A" { @1: FadeIn; }
            par "B" { @1+500ms: FadeIn; }
        }
    }
}
```

This creates two columns with two pieces of text. When the presenter advances the presentation, the
`A` is first shown, followed 500 ms later by the text `B`.
