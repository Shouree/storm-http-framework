Progvis
=======

This is a program visualization tool aimed at concurrent programs and related issues. The tool
itself is mostly language agnostic, and relies on Storm to compile the provided code and provide
basic debug information. The generated code is then inspected and instrumented to provide an
experience similar to a basic debugger. The tool emphasizes a visual representation of the object
hierarchy that is manipulated by the executed program to make it easy to understand how it looks. In
particular, a visual representation is beneficial over a text representation since it makes it
easier to find shared data that might need to be synchronized in a concurrent program.

As mentioned, the tool is aimed at concurrent programs. Therefore, it allows spawning multiple
threads running the same program to see if that affects the program's execution (this is mostly
interesting if global variables are used). Furthermore, any spawned threads also appear in the tool,
and the user may control them independently to explore possible race conditions or other
synchronization errors. If enabled from the menu bar, the tool keeps track of reads and writes to
the data structure in order to highlight basic race conditions in addition to deadlocks.


Starting Progvis
----------------

To start the tool, [download Storm](md://Introduction/Downloads), and click the `Progvis` file
(either `Progvis.bat` or `Progvis.sh` depending on your system). The main window of Progvis should
appear shortly. You can also start Progvis from the interactive Storm prompt by typing
`progvis:main`, or from the command line by typing `Storm -f progvis.main`.


Supported languages
-------------------

In theory, all languages in Storm are supported by Progvis to some extent. As Progvis is a bit
intrusive, however, language support varies slightly:

* `Basic Storm` works well for mostly sequential programs. Progvis does yet not understand the
  standard synchronization primitives and will thus not visualize them properly. Furthermore, a
  suitable visualization is not yet available for all types in the standard library. Some of the
  types there will appear in either too much or too little detail.

* `C` is the main driving force behind Progvis, and as such a larger part of the language is
  supported (but not much of the standard library). Progvis contains its own implementation of C/C++
  in `progvis.lang.cpp`. A large part of the language is implemented and works as intended. The
  implementation is geared towards being used in Progvis, however, and as such pointer safety is
  prioritized above execution speed. As such, the implementation inserts array bounds checks among
  other things to avoid unnecessary crashes, and use after free errors. Type casting is also limited
  to some extent to avoid corrupting memory and crashing the system. The implementation provides a
  set of synchronization primitives that can be used from C that are available in the package
  `progvis.lang.cpp.runtime`. The directory `root/progvis_demo` contains a number of examples that
  show how they are used.

* `C++` is also supported to some extent. The C/C++ frontend implements enough of the language to
  show how central parts of the language behave. For example, pointers, references, classes with
  copy- and move semantics. Most notably, templates and the standard library are currently missing
  from the implementation.
  

