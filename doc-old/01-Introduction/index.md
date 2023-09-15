Introduction
=================

This portion of the documentation serves as a guide for getting Storm up and running on your
machine, as well as illustrating some of the important concepts in Storm by taking a closer look at
the examples included in the download.

Storm currently works on Windows (X86, 32-bit), Linux (X86, and ARM, 64-bit).


Downloading and Installing Storm
----------------------------------

If you are running Debian or Ubuntu, Storm might be available in your system's package manager. If
this is the case, you can simply run `sudo apt install storm-lang`.

First, download Storm from [here](md://Introduction/Downloads) and unpack the archive somewhere convenient. The
archive contains two folders and one executable file:

* `doc` contains the source files for this documentation in markdown format.
* `root` is the directory that Storm uses as the root package when started. The contents of this
  directory is described in greater detail below.
* `Storm` or `Storm.exe` is the main executable for Storm. Start it normally to open an interactive
  command prompt where Storm code can be executed. You can also pass command line parameters to Storm to
  make it do other things than running the interactive command prompt. Use `Storm --help` or
  `Storm -?` to see a list of supported options.

The `root` directory contains the packages loaded by Storm at startup. This is where all Storm code
lives. Each directory here corresponds to a package in Storm. Some of these are described below:

* `demo` contains a couple of examples of what Storm can do. These are discussed further in [Examples](md://Introduction/Examples)
* `lang` is the package where Storm looks for the entry points of any languages it tries to compile.
  There is a sub-package for each supported language, so `lang.bs` contains the entry point used when
  compiling Basic Storm. If you are interested in examining the grammar of the included languages,
  this is where the `bnf`-files containing the grammar are located. Aside from grammar, this package
  contains the implementation of the inline assembler described [here](md://Libraries/Inline_assembler)
  as well as the implementation of quite a few features of Basic Storm.
  The mechanisms used for loading languages are described in more detail [here](md://Storm/Languages).
* `graphics`, `sound` and `ui` contain libraries for playing sound and displaying graphical user
  interfaces. They are documented [here](md://Libraries).
* `present` implements a small program that can be used for making slides for presentations. A
  domain specific language is provided which makes the process more convenient. As such, this
  serves as an example of how Basic Storm can be extended with new functionality. The file `test.bs`
  contains a small example presentation to illustrate how the program is used.
