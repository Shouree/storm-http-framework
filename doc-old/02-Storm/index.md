Storm
=======

Storm is a language that focuses on extensibility on many levels. To be extensible, the Storm compiler
itself is more of a framework for creating languages rather than a complete compiler. Of course, the
compiler includes languages as well, but these are implemented apart from the core itself and could
therefore be moved into external libraries in the future.

The core compiler consists of a type system, to allow different languages to communicate, a parser,
to allow a standard way of defining languages, an assembler, to generate machine code, and a
standard library. Aside from this, the compiler includes two languages: [Basic Storm][1] and a [BNF Syntax][2]
language. If the compiler would not include any languages at all, it would be difficult
to do anything useful with the compiler.

This section will discuss core concepts of the Storm compiler itself. If you are interested in
writing code, please have a look at [Basic Storm][1]. However, the language documentation assumes
you have basic knowledge about how types, threads and to some extent syntax works.

Note that all references to the source code are expressed as file and character offset rather than
file and line number. The offset indicates how many characters into the file the error is present,
CR LF is treated as one character. Use `goto-char` in Emacs to go to the relevant bytes in the file.

Extensibility
-------------

As mentioned before, Storm puts a lot of effort into being extensible. The structure of Storm allows
programmers to easily create new languages, either by transforming languages into an already
existing language, or by going directly at generating machine code. You can even choose which
language within Storm you want to use to implement your new language. Aside from implementing new
languages, Storm also allows and encourages extending languages to suit your needs. This is mainly
done through the built-in parser and the syntax rules present there. The built in parser let you
treat syntax as regular members of a package, so you can mix and match as you see fit for the
problem at hand. Using the syntax language, it is easy to extend an already existing syntax rule of
your favorite language so that it better matches the problem you try to solve.


Threading
----------

The threading model in Storm is based around the idea that you declare which thread to use for
different functions and objects, and let the compiler do the rest. This has at least two benefits:
firstly this makes the intention of threading explicit in the code, and secondly the compiler will
help you to avoid mistakes.

In the compiler, each thread you declare is backed by an OS level thread. Each of these threads will
then be processing messages from other threads. These messages are sent whenever another thread
calls a function that is declared to execute on another thread.  Each of the OS threads are in turn
running cooperatively scheduled user mode threads.  Each incoming message will be launched on its
own thread, and may selectively yield whenever it has to block for some reason (for example when
waiting for a result from another thread).  This way makes recursive calls between threads (e.g. A ->
B -> A) easier to handle, but it does not introduce a lot of possible race-conditions, at least not
worse than regular function calls does.

The programmer may declare actors, non-member functions and global variables to be executed on a
specific thread. When a thread needs to send a message to another thread, all parameters of the
function will be deep copied using the `clone` function, to ensure that there is no shared data
between different OS level threads. The same is true about the return value. Non-member functions
and global variables only support named threads, while actors can be assigned to a specific thread
at runtime.

Threading is explained more in-depth in the [Threads][3] section.


Interactivity
--------------

Another important design goal of the Storm compiler is to make the compiler into a programmers
friend that keeps running alongside your program and helping you along the way, instead of being a
tool that you start whenever you need some work done. Taking this approach, you launch the compiler,
and then the compiler worries about how to build your program, rather than relying on other external
tools for the job. Since the compiler will be running alongside your program, it will be possible to
ask the compiler for information about your program (like "Where is this function declared?"), and
also opens up for features like reloading parts of the running program without restarting it. Taking
this approach, Storm aims to help you to focus on the important parts of your program, rather than
how to build and re-built it efficiently.


[1]: md://Basic_Storm
[2]: md://BNF_Syntax
[3]: md://Storm/Threads
