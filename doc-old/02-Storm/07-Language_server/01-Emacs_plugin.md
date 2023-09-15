The Emacs plugin
=================

The Emacs plugin is included in the normal release of Storm and works with Emacs version 24.4 or
higher. To use it, simply load it from your `.emacs`-file, using `(load "~/path/to/storm-mode.el")`.
Aside from loading the plugin, you need to set two variables which tell the plugin where to find
Storm, and where the directory containing the standard library are located (the root directory included
in the download). This is done using by adding `(setq storm-mode-compiler "~/path/to/Storm.exe")` and
`(setq storm-mode-root "~/path/to/root/")` in your `.emacs`-file after the `load` statement.

After the set up, you can test the plugin by doing `M-x storm-start`, which starts Storm in the
background. You can see the output from Storm by examining the buffer `*compilation*`, which is
hidden by default. If everything works as expected, you should see the text `Language server started.`
there. The plugin will start the Storm process as needed, so you do not have to run `storm-start`
manually in the future. If you wish to stop the language server, run `M-x storm-stop`.

After verifying the setup, you can start using `storm-mode` for files known to Storm. This is done,
like other modes in Emacs, by doing `M-x storm-mode`. If the current file is supported by Storm, you
should see syntax highlighting from Storm while editing the file. If you examine the `*compilation*`
buffer, you should see a message noting that the file has been opened by Storm, or an appropriate
error message on failure.

While it is possible to use `storm-mode` on individual files, or add `storm-mode` to the
`auto-mode-alist` variable, which is usually done with the standard Emacs modes, it is sometimes
convenient to use Storm for all supported files globally. This can be done by enabling
`global-storm-mode`. This mode examines all currently open buffers and all newly opened buffers for
files supported by Storm and uses `storm-mode` for them in that case. This supersedes any other
modes that are applicable for those files. Enabling the global mode can be done by invoking
`M-x global-storm-mode`, which toggles between using and not using the global mode. If you wish
to enable or disable it permanently, put `(global-storm-mode t)` or `(global-storm-mode nil)` in
your `.emacs`-file.

While using `storm-mode`, you can use the following keyboard shortcuts to interact with Storm:
* `C-c u` or `storm-re-color`: update the syntax coloring of the entire file again. This is useful if, for
  some reason, Storm or Emacs missed coloring some characters, or got out of sync. This
  merely transmits the syntax highlighting for the entire file once again, and does not
  attempt to re-parse the file.
* `C-c r` or `storm-re-open`: similar to `storm-re-color`, but parses the file from scratch first.
  This is useful if Storm fails to see the correct parse of a buffer that previously contained many
  or large errors. It could also be required if some edit operations have been lost while interacting
  with the buffer. Usually this rarely happens.
* `C-c h` or `storm-doc`: show documentation on named objects (such as types and functions) in Storm.
  Works similarly to the built-in documentation in Emacs.

Indentation in Storm-mode respects `tab-width` for indentation levels.

Documentation
--------------

The plugin also allows browsing the built-in documentation from Emacs. Run the command `M-x storm-doc`
(bound to `C-c h` in buffers using `storm-mode`) and enter the name of the type or function you wish
to see documentation for. You can use `<tab>` as usual while entering names to auto-complete the name
and to see all available completions. Currently, it is not possible to view documentation for
non-instantiated templates (e.g. `core.Array`). Instead, you need to provide parameters for the
instantiation you are interested in (e.g. `core.Array(core.Int)`). The instantiations that are used
elsewhere in the system will show up in auto-completion just like other types.

In the buffer that shows the documentation, it is possible to navigate to other parts of the documentation
by clicking the highlighted words or the list of members at the end (even though they are not highlighted).
It is also possible to move the cursor to them and press `RET` there to visit them. Use `[back]` and
`[forward]` or `C-c C-f` and `C-c C-b` to go back and forward between previously visited parts of the
documentation.
