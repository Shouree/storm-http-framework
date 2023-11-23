Launching Storm in the Terminal
===============================

How you launch Storm in the terminal depends on your operating system and how you have installed
Storm. Because of this, the remainder of this manual uses the command `Storm` to launch Storm. Since
this may not be the case on your machine, you may have to replace `Storm` with something else on
your machine. This page describes what needs to be changed, if anything.

In all examples below, you are expected to replace `path/to/storm` or `path\to\storm` with the
path that corresponds to where you extracted Storm.


Windows
-------

On Windows, you have the following options to be able to run Storm in a terminal (e.g. cmd.exe, or PowerShell):

- **Navigate to the directory where you installed Storm**

  If you do not aim to develop programs separately from the Storm installation, it is enough to
  navigate to the directory where you installed Storm in your terminal. For example:

  ```
  cd \Path\to\Storm
  Storm
  ```

  This has the benefit that it does not require additional configuration, and is therefore great
  initially when trying out Storm. It does, however, have the drawback that it is not convenient if
  you wish to develop other programs separately.

- **Use the full path to Storm**

  Instead of just writing `Storm` as the command in the terminal, it is possible to replace `Storm`
  with the full path to the Storm binary. If the path contains spaces, you may need to enclose the
  name in double quotes (`"`). For example:

  ```
  C:\Path\to\storm\Storm.exe
  ```

- **Add Storm to the Path variable**

  Finally, you can add the directory where you installed Storm to the environment variable `Path`.
  This means that the system will be able to find Storm just using the name `Storm`. This also
  applies to other places, like the "Run" dialog (when pressing Windows + R). So while it requires
  some initial setup, it makes it much more convenient to use Storm in the future.

  To modify the `Path` environment variable, open the system settings application, search for
  "environment variables" and select "Edit environment variables for your account". This will open a
  dialog that contains a button "Environment variables...". Click the button and another dialog that
  shows the environment variables in the system. Find the list entry `Path` in the topmost list, and
  click "Edit". In the new dialog, add `;C:\Path\to\storm` (note: not the name of the `.exe` file)
  and close all dialogs with OK. You may need to restart your terminal for the changes to take
  effect. After this you should be able to launch Storm just by typing `Storm`.


Linux
-----

If you have installed Storm from your package manager, you do not need to perform any additional
setup to be able to use Storm from a terminal. The only difference is that the Storm binary is all
lowercase rather than capitalized as in this manual.

If you have installed Storm by extracting an archive from this page, you have the following options:

- **Navigate to the directory where you installed Storm**

  If you do not aim to develop programs separately from the Storm installation, it is enough to
  navigate to the directory where you installed Storm in your terminal. Note, however, that most
  systems are configured to require typing `./Storm` rather than just `Storm`. For example:

  ```
  cd ~/Path/to/storm
  ./Storm
  ```

  This has the benefit that it does not require additional configuration, and is therefore great
  initially when trying out Storm. It does, however, have the drawback that it is not convenient if
  you wish to develop other programs separately.

- **Use the full path to Storm**

  Instead of just writing `Storm` as the command in the terminal, it is possible to replace `Storm`
  with the full path to the Storm binary. If the path contains spaces, you may need to enclose the
  name in double quotes (`"`), or escape spaces with a backslash (`\`). For example:

  ```
  ~/path/to/storm/Storm
  ```

- **Add an alias to your shell**

  A simple but non-intruse way to make it more convenient to use Storm is to add an alias to your
  shell. For Bash (which is the default on many systems), this can be done by adding the following
  line to the end of the file `~/.bashrc`:

  ```
  alias Storm=~/path/to/storm/Storm
  ```

  To make the alias visible, either restart your shell, or reload the configuration by typing
  `source ~/.bashrc`.

  Note, that aliases often not visible to scripts.

- **Add Storm to your PATH variable**

  The last option is to add Storm to your PATH variable. This means that all parts of the system
  (running as your user) will be able to find it as if it was a command that was installed on your
  system. This can be done by adding the following line to the file `~/.bashrc`:

  ```
  export PATH="${PATH}:/path/to/storm"
  ```

  To make the alias visible, either restart your shell, or reload the configuration by typing
  `source ~/.bashrc`.
