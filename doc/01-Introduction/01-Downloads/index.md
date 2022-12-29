Downloads
===========

The current version of Storm is `?StormVersion?`, released on ?StormDate?. Only the latest release is
provided in binary form. Earlier releases have to be compiled from source. All releases are marked
in the Git repository by a tag with the name `release/<version>`.

Release notes for each release are found [here](md://Introduction/Downloads/Release_Notes) or in the
annotated release tags in the Git repository.


Binary releases
-----------------

- [Windows (32-bit, X86)](storm_mps.zip)
- [Linux (64-bit, X86)](storm_mps.tar.gz) (should work for recent Debian-based distributions)


To run the compiler, simply unpack the archive file and run `Storm` (`Storm.exe` on Windows), and
the top loop for Basic Storm should start. For more detailed instructions, see
[Introduction](md://Introduction/).

For Windows, no external libraries are required (except for `dbghelp.dll`, which is included with
Windows). The Ui library requires Windows 7 or later.

For Linux, the C standard library for GCC 8.3.0 or later is required. For the Ui library, Gtk+ 3.10
or later is required. `libpng` and `libjpeg` are also required for proper image decoding, but they
are included in the download since there are many incompatible versions of these libraries.


Alternative versions
--------------------

Storm is available with two different garbage collectors. The default garbage collector (used above)
is MPS, which is performant and stable. The other alternative is SMM (Storm Memory Manager) which is
currently in the experimental stage.

- MPS releases

  These releases use the [Memory Pool System](http://www.ravenbrook.com/project/mps/) from
  Ravenbrook Ltd. for memory management. The Memory Pool System is very stable and performant.

  - [Windows (32-bit), MPS](storm_mps.zip)
  - [Linux (64-bit), MPS](storm_mps.tar.gz)

- SMM releases

  These releases use the Storm Memory Manager for memory management, which is a homegrown garbage
  collector for Storm. The SMM generally performs worse compared to the Memory Pool System
  (approximately 2-3x runtimes when using the MPS, and occasionally longer pause times), and it is
  not as mature as the MPS.

  This option is currently experimental, but seems to work well in many cases. It is slower than the
  MPS garbage collector.

  - [Windows (32-bit), SMM](storm_smm.zip)
  - [Linux (64-bit), SMM](storm_smm.tar.gz)



Source releases
----------------

The source code (licensed under GNU LGPL version 2.1) is freely available through Git at the following URL:

`git clone git://storm-lang.org/storm.git`

The repository has a few submodules. To fetch them as well, execute the following commands inside the repository:

`git submodule init`

`git submodule update`


If you get an error about an unreachable submodule, don't worry. That repository only contains some
test data for the language server which is not required.

To build Storm, you need `mymake`, available at [GitHub](http://github.com/fstromback/mymake) or
`git://storm-lang.org/mymake.git`. When you have installed mymake, compiling Storm is just `mm release`
to make a release build. During development, use `mm Main` or `mm Test` to build the
development version of the main entry point and the test suite respectively. Mymake is available as
a package in Debian (testing) as well, and is installed as `mymake` instead of `mm`.

To specify which garbage collector to use, either edit `Gc/Config.h`, or compile storm with `mm mps Main` or `mm smm Main`.

On Linux (Debian), the following packages need to be installed to successfully compile Storm, in addition
to Mymake and GCC:

- `libgtk-3-dev` Gtk+ 3 headers
- `libturbojpeg-dev` Headers for JPEG decoding
- `libpng-dev` Headers for PNG decoding
- `libopenal-dev` OpenAL headers for sound output
- `autotools-dev`, `autoconf`, `libtool`  Build-tools required for building a custom Cairo

These are not required on Windows, as Storm relies on the corresponding functionality in the Windows
API instead of separate libraries.


License
--------

Storm is licensed under the LGPL version 2.1. Note, however, that the MPS release uses the
[Memory Pool System](http://www.ravenbrook.com/project/mps/) from Ravenbrook Ltd. which has a
separate license (BSD 2-clause license). The SMM is licensed under the LGPL version 2.1, just like Storm.

Parts of the libraries, such as the Ui library, may rely on other third party libraries. To check
which libraries are used and which licenses apply, type `licenses` in the Basic Storm top loop, or
call `core.info.licenses` from your code. Note that this only shows loaded libraries. You might want
to use the library you are interested in (e.g. by typing `help ui`) to make sure they are loaded
before querying the license information.
