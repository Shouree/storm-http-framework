Compiling Storm from Source
===========================

The pre-compiled binaries provided on this page are enough for most tasks. However, if you wish to
integrate external libraries into Storm, or change things in the compiler itself, it is necessary to
build Storm from source. Building from source is also an option in case the pre-compiled binaries do
not work on your system for some reason.

The remainder of this page describes how to compile Storm on Windows and Linux. The end of the
document also outlines how to use Mymake to work conveniently with Storm, and how to integrate the
entire flow into Emacs.


Installing Dependencies
-----------------------

The first step is to ensure that all dependencies are installed on your system. Storm is written to
require fairly few dependencies, and the dependencies it does require (apart from Mymake) are in
many cases already installed on your system. In short, Storm requires Mymake (the build system), a
C++ compiler, and a few libraries for graphics and sound on Linux.

### Windows

The following software is needed on Windows. You may already have some of this software installed.
If so, you do not need to install it again, as Storm does not require a particular version of these
tools to build. Storm does not require any additional libraries, so it is enough to install Visual
Studio and Mymake.

- Git

  Git is available for Windows [here](https://git-scm.com/download/windows). Download the installer
  and run it. By default, a minimal unix-like shell environment is also installed, but it is not
  required for building Storm. As long as you can execute `git` commands in a terminal it is enough
  for the purposes of building Storm.

- Visual Studio

  For Windows builds, Storm assumes that it is built with the Visual Studio compiler (Clang may
  work, but it is not tested). The easiest way to install the compiler is to download and install
  [Visual Studio](https://visualstudio.microsoft.com/) (it might be possible to install just the
  toolchain). From the linked page, select "Visual Studio Community" and follow the steps in the
  installer. Note that Visual Studio Code is *not* enough for the purposes of building Storm, as it
  does not include a compiler by default. For the purposes of building Storm, all components except
  for the C++ toolchain are optional, so you may deselect most options if you wish to preserve disk
  space.

  After the installation is complete, it is necessary to start Visual Studio (the IDE) at least once
  and open a C++ project. Otherwise, it might not install the C++ toolchain. To do this, open Visual
  Studio. In the welcome dialog, select "Create a new project". In the next dialog, search for
  "Empty progject" with the language "C++". Next select a location to save the files. The location
  does not matter, as we are only interested in Visual Studio installing the C++ compiler for us.
  After Visual Studio has installed the relevant files, you can close Visual Studio and delete the
  created project.

- Mymake

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
  wish to use. Any version later than Visual Studio 2008 is sufficient. If this fails, ensure that
  you have a "Visual Studio Command Prompt" in the start menu. If not, the C++ compiler has not been
  installed, and you need to try to open a C++ project in Visual Studio again.

  After Mymake has been compiled, the setup asks you if you wish to add it to your Path. This step is
  not technically required, but it will make it easier to build programs with Mymake in the future.

  Finally, ask mymake to create the default configuration file by running:

  ```
  mm --config
  ```

  In a terminal. This process will once again find a Visual Studio compiler to use and set its
  environment variables up accordingly. During this process, Mymake will ask how many threads you
  wish to use when compiling programs. It is a good idea to set this number to the number of cores
  in your machine. If you are unsure about the number of cores in your machine, you can open Task
  Manager, select the Performance tab, select CPU in the left column and find the value "Cores"
  listen to the right. If you are unable to find the number of cores, you can also enter an
  arbitrary number (for example 4 or 8). The only downside of a too low number is that compilation
  will take longer than necessary.

### Linux

On Linux, Storm requires Mymake (the build system), a C++ compiler, Git to fetch the source code,
and a few libraries to interact with the system. As on Windows, Storm is not very sensitive about
the exact versions that are used, so the ones available for your system will likely work. Note,
however, that Storm has only been tested with GCC. It should be possible to build it using Clang as
well, but this has not been tested.

- Installing Packages

  The following packages are required on Linux. For ease of use, the package names used in
  Debian-based systems are given below. The names are likely to vary slightly depending on your
  distribution (e.g. you probably do not need the `-dev` part of the name). To install the packages,
  the following command may be used:

  ```
  sudo apt install \
    build-essential git \
    g++ \
    libssl-dev \
    libgtk-3-dev \
    libpng-dev libjpeg-dev \
    libopenal-dev \
    libmariadb-dev \
    libgl1-mesa-dev \
    libgles2-mesa-dev
  ```

- Mymake

  If you are running Debian, you can install Mymake from the official package repositories (package
  `mymake`). Note, however, that the command is named `mymake` rather than `mm` in that case.

  If you are not on Debian, or wish to compile Mymake from source regardless, the process is as
  below. Note, that if you wish to integrate the build proces into Emacs, you need the file
  `mm-compile.el` from the Mymake repository regardless. That file is not packaged in the Debian
  package.

  First, clone the Mymake repository:

  ```
  git clone https://github.com/fstromback/mymake
  cd mymake
  ```

  Then, run the script `install.sh`. This script will compile Mymake and optionally add an alias for
  it. Note, this script should *not* be run as root:

  ```
  ./install.sh
  ```

  If you wish to handle the process manually, the script `compile.sh` only compiles Mymake. It takes
  a single parameter, the name of the output file. It can be called as follows:

  ```
  ./compile.sh mm
  ```

  If you do not wish to add an alias (as the install script does), you can also copy the compiled
  program (`mm`) to somewhere in your path.

  Regardless of how you installed Mymake, run `mm --config` to generate the default configuration
  file in your home directory. As a part of this process, Mymake will ask how many threads it should
  use when compiling programs. It is a good idea to set this value to the number of cores in your
  machine (or slightly lower if you are low on RAM). If you are unsure about how many cores your
  system has, you can run the following command in a terminal:

  ```
  grep -P 'processor\t*:' /proc/cpuinfo | wc -l
  ```


Downloading the Source Code
---------------------------

The next step is to download the actual source code for Storm. The repository for Storm contains a
number of submodules, so it is easiest to use the following command:

```
git clone --recursive git://storm-lang.org/storm.git
```

If, at a later time, the submodules become out of sync (for example, if they were updated from
upstream), the command `git submodule update` makes sure the proper version of each submodule is
checked out. If a submodule is missing entirely, run `git submodule init` followed by `git submodule
update`.

As of version 0.16.19 of Storm, cloning the entire repository from scratch downloads around 810 MiB
of data and requires around 1.1 GiB of disk space.


### Skipping Submodules

If you wish to reduce the amount of data that is downloaded, or wish to reduce the disk space
required, you can ignore some of the submodules. If you are on Windows, you can ignore the
submodules in the `Linux/` directory. Among others, this directory contains the `cairo` and `skia`
submodule that is around 500 MiB. On Linux, you can likely ignore the `cairo` submodule, as it is
not used by default. To selectively download submodules, it is necessary to clone the repository in
two steps. First, clone the Storm repository without the `--recursive` flag:

```
git clone git://storm-lang.org/storm.git
```

Then initialize all submodules. This also provides a list of the submodules so that it is possible
to deactivate them later:

```
cd storm
git submodule init
```

Then, for each submodule that you don't wish to download, execute the following command. The example
below removes both the `cairo` and the `skia` submodules:

```
git config --remove-section submodule.Linux/cairo
git config --remove-section submodule.Linux/skia
```

(Note: `git submodule deinit` should work, but does not seem to work properly before cloning the submodule.)

When you are done, run `git submodule update --progress` to clone the remaining submodules.


### Compiling Storm

Once all dependencies are installed and the source code is downloaded, Storm can be compiled using
Mymake. To understand how the process works, we need to know a little bit about how Mymake works.

According to Mymake, Storm is a *project* that consists of a number of *targets*. The *project* is
the root of the Storm repository (it contains a file called `.myproject`). Each target is a
subdirectory in the project directory. Each target contains a file named `.mymake` to indicate this.
This means that the directories `Test` and `Main` are *targets*, while `scripts`, `doc` and `root`
ar not targets.

To compile a project, Mymake needs to know which *target* it should produce (this can be specified
in the project file, but this is not the case for Storm). As such, we can compile different parts of
Storm depending on which target we ask Mymake to compile. The two most important targets for Storm
are `Main` and `Test`. The target `Main` produces Storm's main executable (distributed as `storm`),
while the target `Test` compiles the test suite in Storm.

Finally, it is worth knowing that the default behavior of Mymake is to not only compile the selected
target, but to also run it if compilation was successful. As such, it is possible to run and compile
Storm with the following command:

```
mm Main
```

Similarly, the test suite can be compiled and run with:

```
mm Test
```

The commands above both produce binary files in the `debug/` directory. If you do not want Mymake to
execute the created file, the flag `-ne` disables this behavior.

Compiling Storm from scratch typically takes a couple of minutes on recent hardware. However, if you
are building Storm on Linux, it must first compile the Skia library, which can take upwards of 10
minutes, even on recent hardware. This is, however, only done once as Mymake tracks updates to files
and only re-compiles the necessary files.


### Compilation Options

The build files for Mymake have a few configuration options that modify the behavior of the build.
Note that Mymake tracks the command lines used to produce intermediate build files. This means that
Mymake produces a correct build when the build options have changed, even without a `clean` in
between. However, it also means that a large portion of Storm need to be rebuilt when options
change.

The following options are available:

- `mps`

  Use the MPS garbage collector (the default). The `mps` flag produces the files `Storm_mps` and
  `Test_smm` in the current output directory.

- `smm`

  Use the SMM garbage collector (experimental). Produces `Storm_smm` and `Test_smm` files in the
  output directory.

- `release`

  Compile a release version of Storm. This version uses a separate set of build directories to not
  interfere with the debug version. The resulting binaries are placed in `release/` rather than
  `debug/`.

  The release version utilizes slightly more expensive optimizations than the debug version, and
  causes `assert`s to become no-ops among other things. The `release` option also instructs Storm to
  build both the `Main` and the `Test` projects, and to not execute them.

- `dist`

  On Linux: Build Storm to be installed in `/usr/bin`. This means that the logic for finding the
  `root` directory is different than normal.

- `noskia`

  On Linux: Do not compile and link Skia to the UI library. This does, however, mean that hardware
  accelerated graphics will not be available.

- `localmariadb`

  On Linux: use the local copy of the MariaDB headers instead of the ones installed in the system.
  This makes it possible to build Storm without `libmariadb-dev`, but may cause compatibility issues
  if your system ships a version of the MariaDB client library that is too different from the one in
  the source repository.

- `usecairogl`

  On Linux: Enable the experimental OpenGL mode in Cairo for the UI library. This will cause Mymake
  to compile Cairo from source, which can take some time. Since this mode is not officially
  supported by Cairo, it may be removed in the future.

- `nostatic`

  Do not use the libraries in the repository where possible, but rely on system libraries instead.
  This means that the following libraries will have to be installed in addition to the ones
  mentioned above: `libmpg123-dev`, `libogg-dev`, `libflac-dev`, and `libvorbis-dev`.


### Common Errors

Mymake utilizes *precompiled headers* to speed up the build process. This is generally positive, but
can cause issues in some cases. In particular, the precompiled header files are closely tied to the
exact version of the compiler that is being used. This means that the build process might fail
whenever the compiler has been updated. Typically this results in a message that says that a "pch
file" or "precompiled header" is invalid. This error is resolved by deleting all files with the
extension `pch` (on Windows) and `gch` (on Linux). In a Unix-like shell, this can be done as
follows:

```
find . -name "*.gch" -exec rm \{\} \;
```


Using Mymake to Run Storm
-------------------------

When developing Storm with Mymake, it is often convenient to let Mymake handle both compilation and
execution of Storm. This means that it is enough to keep track of a single command line rather than
separate ones for compiling and running the program.

To do this, it is, however, often necessary to pass parameters to Storm. This can be achieved by
adding a `--` on the command line, followed by the command line parameters that should be given to
Storm. For example, to pass the parameters `-f progvis.main` to Storm upon successful compilation,
one can invoke Mymake as follows:

```
mm Main -- -f progvis.main
```

Integration in Emacs
--------------------

...
