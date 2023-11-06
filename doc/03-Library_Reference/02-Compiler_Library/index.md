Compiler Library
================

Storm provides a number of utilities that aid in creating languages, and compilers for such
languages. These utilities can be roughly divided into three parts:

1. A library of [named entities](md:/Language_Reference/Storm/The_Name_Tree) that are stored in the
   *name tree*. These form a common representation of central concepts that can be shared between
   languages. Examples are functions and types.

2. A library for parsing text into abstract syntax trees. The conceptual part of this area of the
   compiler library is covered in the section on [the syntax
   language](md:/Language_Reference/The_Syntax_Language). This part of the manual covers the
   technical parts, such as details of the `Parser` class.

3. A library for generating machine code. Again, the conceptual parts of this area of the compiler
   library is covered in the section on the [intermediate
   language](md:/Language_Reference/Intermediate_Language). This part of the manual is, however,
   more technical in nature.
