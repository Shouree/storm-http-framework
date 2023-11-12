Presentation Library
====================

The [stormname presentation] package contains a library for creating presentations (i.e.
slideshows). It consists of two parts, a library that implements data structures and logic for
creating presentations, and a domain specific language for Basic Storm that makes it more
convienient to create presentations. The domain specific language is based on the language provided
by the [layout library](md:/Library_Reference/Layout_Library).

The documentation for each of these two parts are split in two. The first part covers how to create
slides using the basic layout system. The second part details the *picture* subsystem that is
designed to create more intricate drawings. The picture system is inspired by TikZ in LaTeX.


Creating a Presentation
-----------------------

It is usually a good idea to create a presentation as a package (i.e. a directory on disk
somewhere). By creating a `main` function in the package, it is then possible to launch the
presentation easily from the command line using:

```
Storm <path-to-package>
```

The `main` function then needs to create a [stormname:presentation.Presentation] object and call the
`show` function to create a window. Creating a presentation is often done using the [presentation
syntax](md:Syntax) in Basic Storm.

The `show` function views the presentation in *release mode*. This means that the library
pre-compiles all code in the presentation library before the presentation is shown. This has the
benefit that stuttering due to compilation are minimized, but it might not be convenient when
developing the presentation. For this reason, there is also a *debug mode* that skips compilation,
and allows hot reloading of the presentation. This mode is accessing by retrieving the named entity
of the function that creates the presentation, and then calling `showDebug`.

To summarize, a simple presentation may look like this:

```bs
use presentation;

presentation Demo "Demo" {
    slide title "Demo Presentation", "Author" {}
}

void main() {
    Demo.show();
}
```

Or, for debug mode:

```bs
use presentation;
use lang:bs:macro; // For named{}

presentation Demo "Demo" {
    slide title "Demo Presentation", "Author" {}
}

void main() {
    showDebug(named{Demo});
}
```


Controlling the Presentation
----------------------------

The window created by the `show` or `showDebug` functions only contains a view of the presentation.
The mouse cursor is hidden in the window. There is, however, possible to highlight things by
clicking and dragging the mouse (typically most convenient with a touchscreen). This shows a red
circle where the mouse is clicked, which can be used to highlight things. Furthermore, individual
components can choose to show a custom cursor when the mouse is over them.

Apart from the pointer, the presentation is controlled entirely using the keybord. The following
commands are available:

- `q` or *escape*: Close the presentation window.

- *right arrow* or *down arrow*: Go to the next step in the presentation (i.e. next animation or
   next slide).

- *left arrow* or *up arrow*: Go to the previous slide in the presentation.

- `f`: Toggle fullscreen mode. The window enters fullscreen on the display it is currently on. So to
  move it to an external display, first move it to the proper display while windowed, and then press
  `f`.

- *home*: Go to the beginning of the presentation.

- *end*: Go to the end of the presentation.


When running in *debug mode*, the following keybindings are also available:

- `F5`: Reload the package where the presentation was defined, execute the supplied function again,
  and use the new version. This allows updating the presentation without restarting the entire
  viewer program.

- `F8`: Show a summary of all threads in the system. Useful to debug resource leaks when integrating
  other tools into the presentation library.



Resources to a Presentation
---------------------------

It is not uncommon to use external resources in a presentation, such as images. One way to manage
such resources is to put them inside a subdirectory (e.g. `res`), and then use code like below to
access them. This makes it possible for the library to locate the resources regardless of how Storm
was started, and where the presentation is located in the file system.

```bs
use lang:bs:macro;
use core:io;

Url resUrl(Str name) {
    if (url = named{}.url) {
        return url / "res";
    } else {
        // Should not happen.
        return Url();
    }
}
```
