Compiling Storm from Source
===========================

The pre-compiled binaries provided on this page are enough for most tasks. However, if you wish to
integrate external libraries into Storm, or change things in the compiler itself, it is necessary to
build Storm from source. Building from source is also an option in case the pre-compiled binaries do
not work on your system for some reason.

The remainder of this page describes how to compile Storm on Windows and Linux. The end of the page
also contains information on how the compilation process can be integrated into Emacs.


Compiling Storm on Windows
--------------------------

To build Storm on Windows, Git, Visual Studio, and Mymake are required. You may already have some of
these tools installed. If so, you do not need to install them again.

### Git

Git is available for windows [here](https://git-scm.com/download/windows). Download the installer
and run it. By default, a minimal unix-like shell environment is also installed, but it is not
required for building Storm. As long as you can execute `git` commands in a terminal it is enough
for the purposes of building Storm.

### Visual Studio

A C++ compiler is necessary to build Storm. The easiest way to install one is to download and
install [Visual Studio](https://visualstudio.microsoft.com/). Note that Visual Studio Code is *not*
enough for the purposes of building Storm, as it does not include a compiler by default. From the
linked page, select "Visual Studio Community" and follow the steps in the installer. For the
purposes of building Storm, all components except for the C++ toolchain are optional, so you may
deselect most options if you wish to preserve disk space. After the installation is complete, it is
necessary to start Visual Studio (the IDE) once, and open a new empty C++ project (command-line
based). This step is necessary to make Visual Studio install the compiler.

You do not need the latest version of Visual Studio to build Storm. Visual Studio 2008 or later is
enough.


### Mymake

Storm uses mymake as its build system. Mymake is available
[here](https://github.com/fstromback/mymake). To install it, first clone the repository into some
directory:

```
git clone https://github.com/fstromback/mymake
cd mymake
```

Then, run the file called `setup.exe` in the root (either from the command-line or from the GUI).
This program will attempt to find the Visual Studio compiler automatically and use that to compile
Mymake from source. If it finds multiple versions of Visual Studio, it will ask you which one you
wish to use. Any version later than Visual Studio 2008 is sufficient. If this fails, ensure that you
have a "Visual Studio Command Prompt" in the start menu. If not, the C++ compiler has not been
installed, and you need to try to open a C++ project in Visual Studio again.

After Mymake has been compiled, the setup asks you if you wish to add it to your Path. This step is
not technically required, but it will make it easier to build programs with Mymake in the future.

Finally, ask mymake to create the default configuration file by running:

```
mm --config
```

In a terminal. This process will once again find a Visual Studio compiler to use and set its
environment variables up accordingly. During this process, Mymake will ask how many threads you wish
to use when compiling programs. It is a good idea to set this number to the number of cores in your
machine. If you are unsure, open Task Manager and look at the Performance tab under CPU.


### Downloading the Storm Source Code



### Compiling Storm

Storm requires no additional dependencies on Windows:

```
mm Main
```



Compiling Storm on Linux
------------------------