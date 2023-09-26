Downloads
===========

Binary releases for Storm are provided below. Only the latest version is provided in binary form.
Earlier versions need to be compiled from source. All reseases are marked by a tag with the name
`release/<version>` in the Git repository.

Release notes for each version can be found [here](md:/Downloads/Release_Notes), or in the
annotated tags in the Git repository.

Storm is licensed under the 2-clause BSD license from version 0.6.19 and onwards. Type `licenses` or
`fullLicenses` in the Storm prompt for more details.


Binary releases for ?StormVersion? (?StormDate?)
--------------------------

- [Windows (32-bit, X86)](storm_mps_win32.zip)
- [Windows (64-bit, X86)](storm_mps_win64.zip)
- [Linux (64-bit, X86)](storm_mps_amd64.tar.gz) (should work for recent Debian-based distributions)
- [Linux (64-bit, ARM)](storm_mps_arm64.tar.gz) (should work for recent Debian-based distributions)


Simply download and unpack the archive somewhere. Then run the `Storm` (`Storm.exe` on Windows) file
to start the interactive top loop. There are also scripts to start bundled applications. For example
`Progvis.sh`/`Progvis.bat` to start Progvis conveniently.

On Windows, the downloaded executables should work on Windows 7 and later. They only depend on files
that are typically installed alongside Windows.

On Linux, the binaries depends on the C and C++ standard libraries for GCC 8.3.0 and later. For the
Ui library, Gtk+ 3.10 or later is required. If you wish to connect to MariaDB or MySQL databases,
you also need to install the MariaDB client library (`libmariadb.so.3`).

A detailed tutorial for installation in different scenarios is available in
[the tutorial](md:/Getting_Started/Installing_Storm).


Alternative versions
--------------------

Storm is available with two different garbage collectors. The default garbage collector is the MPS,
which is performant and stable. Another collector, SMM (Storm Memory Manager) is also available. It
is, however, currently in the experimental stage and is not currently stable. For example, it
crashes when running the full Storm test suite. Below are binary releases for two platforms for
those that wish to experiment:

- [Windows (32-bit), SMM](storm_smm_win32.zip)
- [Linux (64-bit), SMM](storm_smm_amd64.tar.gz)


Source Releases
---------------

The source code is freely available through Git at the following URL:

`git clone git://storm-lang.org/storm.git`

The repository contains a few submodules. After cloning the repository, you need to fetch the
submodules as well:

`git submodule init`

`git submodule update`

Instructions on compiling Storm from source are available in [the getting started section](md:/Getting_Started/Installing_Storm).


License
--------

Storm is licensed under the 2-clause BSD license. Note, however, that some libraries used by the
system come with different licenses. To check which libraries are used and which licenses apply,
type `licenses` at the interactive Basic Storm prompt, or call `core.info.licenses` from your code.
Note that this only shows loaded libraries. You might want to use the library you are interested in
(e.g. by typing `help ui`) to make sure they are loaded before querying license information.

Also note that some programs, most notably Progvis, have different licenses. Check the About menu
option for details.
