Emacs Integration
=================

To improve the development experience, it is useful to integrate Storm and Mymake into your editor
or IDE. Currently, the Storm repository only contains pre-made solutions for integrating Storm with
Emacs. This is, of course, possible to do with other editors as well based on the information
available on this side, but no pre-made solutions exist yet. Please [contact
me](mailto:info@storm-lang.org) if you have developed integrations with other editors, or are
interested in doing so.


The Emacs integration is able to provide the following functionality:

1. Jumping to the location associated with error messages.
2. Accessing the built-in documentation in Storm.
3. Providing syntax highlighting for all languages in Storm.
4. Easy compilation with Mymake.

Each of these are explained in more detail in separate sections below the description of how the
plugin is installed.


Installing the Plugin
---------------------

To use the functionality mentioned above, you need to load the Storm plugin into Emacs. The plugin
is distributed in the binary releases of Storm provided on the webpage as `storm-mode.el`, or in the
source repository as `Plugin/emacs.el`.

### Loading the Plugin

To use the Emacs integration for anything, you must first load the plugin. To do this, open the file
`~/.emacs` and add the following to the end of it (replace the path with the actual path to the
`storm-mode.el` file):

```
(load "~/storm/storm-mode.el")
```

To apply the changes, either place the cursor at the end of the line and press `C-M-u`, or restart
Emacs. If you only wish to jump to errors (step 1 above), this is all the setup of the plugin you
need. You might, however, wish to look at the section on syntax highlighting below to make it more
convenient to edit Basic Storm code.

### Configuring the Plugin

If you wish to access the built-in documentation in Storm, or wish to use the language server, you
must also tell the plugin where Storm is located, and where the root of the name tree is located.
This is done by setting the variables `storm-mode-compiler` and `storm-mode-root`. To do this, add
the following to the end of the `~/.emacs` file:

```
(setq storm-mode-compiler "~/storm/storm")
(setq storm-mode-root "~/storm/root")
```

The paths in the lines above need to be modified to match where Storm is located on your system. The
path in `storm-mode-compiler` is expected to refer to the Storm binary you downloaded or compiled.
If you installed Storm through the system's package manager, you can use `which storm` to find its
location on your system. If you compile Storm using Mymake, Storm will be located in `debug/Storm`
relative to the root of the Storm repository.

As above, you need to apply the changes by either pressing `C-M-u` after each line, or by restarting
Emacs.

The path in `storm-mode-root` is expected to refer to the `root` directory that you wish to use. The
one used by Storm can be found by simply starting Storm in the top-loop. Storm prints the path of
the root directory as a part of its greeting message.

### Integrating Mymake

Finally, if you wish to compile Storm from source, and to use Mymake to build and run Storm, you
also need to install the Mymake plugin from the Mymake repository. This file is not distributed with
the binary packages in Debian, so you need to download the file separately regardless.

If you have built Mymake from source, you can load the file `mm-compile.el` from the repository you
have already cloned. If you installed Mymake from the system's package manager, you need to download
the file from the [Mymake
repository](https://github.com/fstromback/mymake/blob/master/mm-compile.el) and place it somewhere
in your file system.

To load the Mymake plugin, add the following to your `~/.emacs` file, right before the line that
loads `storm-mode.el`. Note that the Mymake plugin will add a few global keybindings by default.

```
(load "~/mymake/mm-compile.el")
(setq mymake-command "mm")
```

The path in the first line needs to be modified to reflect the location of the file `mm-compile.el`
on your system. The second line specifies the name of the Mymake binary on your system. If you added
Mymake to your system's Path (so that you can run it by typing `mm` anywhere), then "mm" as
specified above is enough. If this is not the case, you can specify the full path to Mymake here.
Note: on Debian, the Mymake command is named `mymake` rather than `mm`.

As above, you need to apply the changes by either pressing `C-M-u` after each line, or by restarting
Emacs.


Jumping to Errors
-----------------

When the plugin is loaded, it adds a few patterns to `compilation-mode` to recognize error messages
in Storm, and allows jumping to them easily. To use this functionality, you need to use the command
`compile` in Emacs to run Storm. This can be done by pressing `M-p`, typing `compile` and then
pressing Enter. This makes Emacs ask you which command to use for compiling. Remove the default
(often "make -k"), and instead type `storm <file>`, or whichever command you wish to run and press
Enter again. This causes Emacs to display a buffer named `*compilation*` that displays the output of
Storm.

If Storm prints an error message, the source reference in the error message will be underlined and
highlighted in a different color than the rest of the text. This means that it is possible to jump
to the error by pressing `M-x`, typing `next-error` and then pressing Enter. This causes Emacs to
open the file that contains the error, and to jump to the correct position in the file. Emacs also
highlights the problematic piece of text for a short while.

Of course, it is inconvenient to use the full commands each time. For this reason, it is convenient
to bind them to keys. For example, to bind `next-error` to the key `M-n`, the following can be added
to the end of `~/.emacs` (press `C-M-u` after the line to apply it immediately, or restart Emacs):

```
(global-set-key (kbd "M-n") 'next-error)
```

If you do *not* plan on using Mymake to compile Storm, it might also be worth binding `compile` to a
key, for example `M-p` (press `C-M-u` after the line to apply it immediately, or restart Emacs):

```
(global-set-key (kbd "M-p") 'compile)
```

Note that you only need to enter the command the first time you use the `compile` command. Emacs
remembers the command you entered, so the next time you can immediately press Enter when Emacs asks
for the command line.

If you loaded `mm-compile.el`, then the Mymake plugin provides an alternative to the `compile`
command that is already bound to `M-p`. So you do not need to bind it yourself.


Accessing the Built-In Documentation
------------------------------------

If you set the variables `storm-mode-compiler` and `storm-mode-root`, the Storm plugin also allows
browsing the built-in documentation directly in Emacs. To access the documentation, type `M-x
storm-doc` and press Enter. Emacs will then ask you for a name to display documentation about. Note
that this name should be written using the generic Storm syntax (i.e. using `.` and not `:`). Tab
can be used for auto completion as usual.

For example, to read documentation about the string class, type `M-x storm-doc` and press Enter. In
the new prompt, type `core.Str` and press Enter again. This will open up a new buffer that contains
the documentation. In contrast to the textual documentation in the top-loop, the page in Emacs
contains clickable hyperlinks in many places. This makes it convenient to read the documentation
about the members in the class, for example.

Note that the Storm process started by Emacs does not currently reload code automatically. This
means that documentation for code you are currently writing may not be available. This can be
resolved by restarting the Storm process, which is done by typing `M-x storm-restart` and pressing
Enter.

Both of these commands can of course be bound to keys similarly to above if desired.


Syntax Highlighting
-------------------

If you do not wish to use the language server for syntax highlighting, it is a good idea to at least
tell Emacs to use `java-mode` for Basic Storm source code. This allows Emacs to provide some level
of syntax highlighting and indentation, since Basic Storm is syntactically similar to Java. This can
be done by adding the following to the end of the file `~/.emacs`. This does *not* conflict with the
use of the language server, so it can act as a good fallback regardless.

```
(add-to-list 'auto-mode-alist '("\\.bs$" . java-mode))
```

As above, press `C-M-u` after the line or restart Emacs to apply the changes.


To use the language server to perform syntax highlighting in a buffer, go to the buffer and press
`M-x`, then type `storm-mode`, and finally press Enter. This makes the buffer use the language
server for syntax highlighting.

If you wish to automatically use the language server for all supported file types, you can press
`M-x`, type `global-storm-mode`, and press Enter. This makes Storm mode the default for the current
session. If you wish to make it permanent, you can add the following to the end of your `~/.emacs`
file:

```
(global-storm-mode t)
```


### Troubleshooting

In some cases, the syntax highlighting will not work properly. Any error messages are outputted to
the `*compilation*` buffer, which may help to narrow the problem down. Two common issues are:

- Syntax highlighting in a file fails to work at all

  In order to support syntax extensions in the same directory as the current file, the language
  server will attempt to load all files in the same directory as the file before attempting to
  perform syntax highlighting. If this fails due to a syntax error in one of the files, syntax
  highlighting will fail, and an error message is printed to the `*compilation*` buffer. You can
  typically jump to the error by using `next-error` (bound to `M-p` above). After fixing the error,
  you currently need to restart the Storm process by pressing `M-x`, typing `storm-restart`, and
  then pressing Enter. Note that this only happens the first time a file in a directory is
  highlighted. As such, as long as you ensure that files parse properly when you close Emacs, this
  is typically not a problem.

- Syntax highlighting is offset by a few characters

  This happens at times when the language server lags behind the typing of the user. The protocol
  aims to compensate for this type of lag, but there is apparently some bugs remaining.

  If this happens, you can simply press `C-c r` to re-open the file and re-do syntax highlighting.



Compiling with Mymake
---------------------

If you loaded the `mm-compile.el` plugin, you can also compile Storm with Mymake. This is convenient
when developing code in C++ alongside code in Storm. It is also convenient for pure Storm
development, since the Mymake plugin provides a mechanism to store the compilation command in a
file. Using Mymake does, however, require that you are willing to compile Storm from source, as
there is no easy way to instruct Mymake to just run Storm without the C++ source code available.

By loading the `mm-compile.el` file, the following keybindings are added globally:

- `M-p` - compile the code in the current buffer. See below how this is done.
- `C-c q` - compile the code in the current buffer, specify a command manually.
- `C-c C-k` - kill the current compilation process.

When you ask the Mymake plugin to compile code, it will search the directory of the current buffer
and upwards for a file named `buildconfig`. If such a file is found, it uses the directory that
contains the file as the current working directory and examines the contents of the file. Any lines
that start with `#` are considered to be comments. Any remaining text are interpreted as command
line parameters to Mymake.

If a file named `buildconfig` was found, then the plugin executes Mymake in that directory with the
parameters specified in the file. This means that it is possible to store the parameters to run the
code you are currently developing in the `buildconfig`-file in the root of the Storm project. Doing
so allows you to compile the code by pressing `M-p` anywhere in the Storm repository to compile and
run your code. Since the compilation is run inside Emacs, you can also jump to any compilation
errors that appear in the compilation by pressing `M-n` (based on your setup above).

The Storm repository contains a `buildconfig` file at the root that contains a number of commands
for launching various things. Most of them are commented out, but they act as a reference for how
commands can be formatted. The `buildconfig` file in the Storm repository also contains some options
for automatically generating a skeleton in newly created C++ files. Refer to the documentation in
the `mm-compile.el` file for more details on this.

If no `buildconfig`-file is found, the Mymake plugin simply executes Mymake in the current
directory, but with the flag `--default-input <filename>`. This causes Mymake to look for a suitable
`.mymake`- or `.myproject` file in the current or parent directories, and compile the code based on
the information there. If no such file is found, or if the file does not specify an input file, the
file provided after `--default-input` is used. The Mymake plugin supplies the name of the currently
open file here, which makes it possible to use `M-p` to compile everything from standalone files, to
large projects like Storm conveniently.
