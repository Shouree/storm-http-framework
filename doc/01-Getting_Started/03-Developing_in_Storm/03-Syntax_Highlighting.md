Syntax Highlighting
===================

When writing code for Storm, it is useful to set up syntax highlighting and automatic indentation to
make the process easier.

The easiest option for languages like Basic Storm is to tell your editor that `.bs` files should be
highlighted like Java source code. This works well since the surface level syntax of Basic Storm is
similar to that of Java (at least without extensions). How to do this varies from editor to editor.
In Emacs, this can be done by adding the following to the file `~/.emacs`:

```
(add-to-list 'auto-mode-alist '("\\.bs$" . java-mode))
```

The Language Server
-------------------

When working with languages other than Basic Storm, or when using certain extensions to Basic Storm,
the built-in capabilities of the editor may not be enough. In such cases it is better to let the
editor rely on information from Storm itself through the language server.

To use the language server, the Storm plugin from the file `storm-mode.el` from the Storm releases
must be loaded. The process is the same as the one described [here](md:Source_References). In short,
add the following line to the end of the file `~/.emacs` to load the plugin:

```
(load "~/storm/storm-mode.el")
```

In addition to loading the plugin, we also need to tell the plugin where Storm is located, and where
its `root` directory is located. This can be done by adding the following lines after the `load`
line (modify the paths according to where you placed Storm on your system):

```
(setq storm-mode-root "~/storm/root")
(setq storm-mode-compiler "~/storm/storm")
```

The value for `storm-mode-root` can be retrieved by starting the interactive top-loop. It is printed
as a part of the greeting message. The value for `storm-mode-compiler` can be retrieved using the
command `which storm` on Linux systems if you are unsure where you installed it (this will not work
if you made it into an alias, in that case type `alias` and see the path from there).


When you have added the lines above to your `~/.emacs` file, restart Emacs (or type `M-x
eval-buffer`) to apply the changes. The plugin is now able to provide a mode that uses Storm to
perform syntax highlighting. To use the mode, open a Storm file and type `M-x storm-mode` followed
by Enter to enable it.

If you wish to use `storm-mode` for all file types supported by Storm, you can run `M-x
global-storm-mode` to enable it for the current session. If you wish to make it the default for
future sessions as well, put the following at the end of your `~/.emacs` file:

```
(global-storm-mode t)
```


Documentation in Emacs
----------------------

The Storm plugin also allows browsing the built-in documentation directly in Emacs. To access the
documentation, type `M-x storm-doc` and press Enter. Emacs will then ask you for a name to display
documentation about. Note that this name should be written using the generic Storm syntax (i.e.
using `.` and not `:`). Tab can be used for auto completion as usual.

For example, to read documentation about the string class, type `M-x storm-doc` and press Enter. In
the new prompt, type `core.Str` and press Enter again. This will open up a new buffer that contains
the documentation. In contrast to the textual documentation in the top-loop, the page in Emacs
contains clickable hyperlinks in many places. This makes it convenient to read the documentation
about the members in the class, for example.

Note that the Storm process started by Emacs does not currently reload code automatically. This
means that documentation for code you are currently writing may not be available. This can be
resolved by restarting the Storm process, which is done by typing `M-x storm-restart` and pressing
Enter.
