Tutorials
=========

This section contains a number of tutorials. Each tutorial illustrates how to accomplish some
(simple) task in Storm in a step-by-step fashion. Each step describes the rationale behind the step,
and how the program can be executed and tested at that step. The final step of each tutorial is
present in the directory `root/tutorials` of the source repository and the binary distributions.
They are, however, not available in the Debian release yet.

The following tutorials are available. They are roughly ordered in level of complexity, so it is a
good idea to start from the topmost one and work downwards:

- [**Fundamentals**](md:Fundamentals)

  Shows basic concepts when working with Storm, such as text IO, debugging tips, and some common
  constructs.

- [**Values and Classes**](md:Values_and_Classes)

  Illustrates the differences between value types and class types through examples.

- [**Threading**](md:Threading)

  Shows how the [threading model](md:/Language_Reference/Storm/Threading_Model) in Storm works, and
  some implications that might be surprising for new users of Storm.

- [**Files and Streams**](md:Files_and_Streams)

  Shows how the [IO library](md:/Library_Reference/Standard_Library/IO) in Storm can be used to
  access files in the file system, and how to read and write text files.

- [**Serialization**](md:Serialization)

  Shows how [serialization](md:/Library_Reference/Standard_Library/IO/Serialization) works in Storm,
  and how it can be used to implement a simple network protocol.

- [**UI**](md:UI)

  Shows how to build a simple graphical application in Storm using the [UI
  library](md:/Library_Reference/UI_Library). The tutorial also illustrates how the [layout
  library](md:/Library_Reference/Layout_Library) can be used to simplify the task of positioning
  elements in a window.

- [**Tests**](md:Tests)

  Shows how the [unit test library](md:/Library_Reference/Unit_Tests) can be used to write tests for
  code in Storm.

- [**Using Grammar**](md:Using_Grammar)

  Shows how to use the [syntax language](md:/Language_Reference/The_Syntax_Language) to parse text.
  Transforms are then used to extract the relevant parts of the parse tree and perform simple
  operations on it.

- [**New Expressions in Basic Storm**](md:New_Expressions)

  Shows how to create simple language extensions to Basic Storm (and other languages). This
  illustrates how the extensibility of the syntax language works, and shows how Basic Storm uses it
  for extensibility.

- [**New Entities in the Name Tree**](md:New_Entities)

  Shows how to create new entities in the name tree, and how to create a syntax extension to Basic
  Storm to easily create them. This both shows how to extend the name tree, and how to add new
  constructs to the top-level parts of Basic Storm.

- [**New Languages in Storm**](md:New_Languages)

  Shows how to implement a new but small language in Storm. This illustrates how Storm loads new
  languages, and illustrates how one would start implementing a new language in Storm.

- [**Libraries in C++**](md:Libraries_in_C++)

  Describes how to create a library with code written in C++, and how to integrate it into the
  compilation process of Storm.
