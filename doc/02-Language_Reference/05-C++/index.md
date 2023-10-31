C++ with Storm
==============

It is possible to use C++ code with Storm. However, Storm is not currently able to compile C++ code
(apart from the minimal frontend for [Progvis](md:/Programs/Progvis). Therefore, integration with
C++ code is more involved than for other languages, and relies on an external C++ compiler being
used. However, with some care the C++ integration present in Storm allows good integration between
C++ and Storm. From Storm's perspective, it is completely transparent whether code is written in C++
or Storm. The reverse is, however, not true since the standard C++ compiler is not aware of code
written in Storm.

This section of the manual details how code in C++ is written, how it is compiled, and the
constraints of the integration.
