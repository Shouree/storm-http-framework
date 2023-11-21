Installing Storm
================

This page describes how to install Storm on your system. For most users, it is enough to install the
pre-built binaries. This allows running programs written in Storm, and to develop new programs in
Storm. If you intend to develop programs in Storm, you might want to have a look at the section
[developing in storm](md:/Getting_Started/Developing_in_Storm), which describes how to set up a
convenient development experience.


Windows
-------

The Windows version of Storm assumes that you have a system with an x86-based CPU and are running on
Windows 7 or later (Windows 8 or later if you wish to use SSL connections).

The [downloads](md:/Downloads) page provides two archives for Windows, one for 32-bit systems and
one for 64-bit systems. To install Storm, simply unpack the downloaded archive to a location you
find convenient, and run `Storm.exe`.

The first time you start Storm, you may see a dialog with the title "Windows has protected your PC".
This is shown because Windows stores that the file originates from the Internet, and the file has
not been digitally signed. To run Storm, click the link titled "More info", and then the button "Run
anyway".

The applications listed in the manual are also available as `.bat`-files. For example, to start
Progvis, launch `Progvis.bat`.


Linux
-----

Storm requires a 64-bit CPU to run on Linux. Storm supports both x86-64 (AMD64) and ARM (ARMv8) CPUs
on Linux, and is thus able to run on everything from desktops to small single-board computers.

To install Storm, you have two options:

- **Installing from the Webpage**

  The binaries on the Storm webpage are built using the `oldstable` version of Debian. They are
  therefore known to work on recent Debian-based systems (e.g. Ubuntu). They have also been tested
  on other distributions, such as Arch Linux, and may work there as well.

  The binary releases contain everything that is necessary, except for core system libraries that
  are likely already installed on your system. Therefore, installing Storm is as simple as
  downloading the appropriate release from the [downloads](md:/Downloads) page, unpacking it, and
  then running the `Storm` executable.

  **Note:** Chrome or Chromium has a habit of automatically uncompressing `.tar.gz`-files. This
  causes some archive managers to be confused. If this happens to you, simply rename the file from
  `.tar.gz` to `.tar` and try again.

  On the command-line, this can be done as follows:

  ```
  # Create a directory where you wish to install Storm:
  mkdir ~/storm
  cd ~/storm

  # Download the archive:
  # For x86-64/amd64:
  wget https://storm-lang.org/storm_mps_amd64.tar.gz
  # For ARM:
  wget https://storm-lang.org/storm_mps_arm64.tar.gz

  # Unpack the archive:
  tar xf storm_mps_*.tar.gz

  # Launch Storm:
  ./Storm
  ```

  The archive also contains `.sh`-files to start the applications described in this manual. For
  example, to launch Progvis, run `Progvis.sh`.

  If Storm starts, but crashes when you import certain libraries, make sure that you have Gtk3 and
  OpenSSL installed. If Storm does not start at all, you might need to [compile it from
  source](md:/Getting_Started/Developing_in_Storm/Compiling_from_Source).


- **Using the Package Manager**

  If you are running a recent Debian-based system, you can install Storm from the system's package
  manager. Note, however, that due to the release cycles of Debian-based systems, the version you
  will get this way is likely a bit older than what is available through this page. This is usually
  not a problem, but something to be aware of in case something on the Storm webpage does not match
  what is available in your installation.

  To install from the package manager, simply run the following command in a terminal:

  ```
  sudo apt install storm-lang
  ```

  By default, this will install Storm itself, and all libraries that are mentioned on this page. It
  is also possible to install different libraries separately if desired (the packages are named
  `storm-lang-X` where `X` is the name of the library).

  After this, you can start Storm by typing `storm` in your terminal.

