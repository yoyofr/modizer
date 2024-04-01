
# gbsplay INSTALL instructions

For the impatient: Simply run `make`.  For details see below.

## PACKAGE INSTALLS

gbsplay is available as a pre-compiled package on many Linux
distributions.  Using these packages will usually be the easiest way
to install.  Please note that these packages might be a somewhat
outdated as we don't release very often.

A list of packages can be found at
<https://repology.org/project/gbsplay/versions>.

On FreeBSD, gbsplay can easily be compiled from ports:
<https://www.freshports.org/audio/gbsplay/>

## BUILDING FROM SOURCE

You can also build gbsplay from source.
You might want or need to do this if

- there are no packages for your system available
- you want to use the latest features and/or bugfixes
- you want to edit the source code

gbsplay expects a unix-style build environment for C code.  If you
have already compiled other software from C sources, you should be
good to go.  At the very least the following is needed:

- a C compiler (we regularly test with both `gcc` and `clang`)
- the corresponding `binutils`
- GNU `make`
- `bash`

To get the the sources, either clone our repository using

```sh
git clone https://github.com/mmitch/gbsplay.git
```

or by downloading <https://github.com/mmitch/gbsplay/archive/refs/heads/master.zip>

### SUPPORTED SYSTEMS

gbsplay should run on nearly every Linux distribution (see the package
list above).  It should also run on architectures different from
i686/amd64 (the official Debian packages are built on all
architectures supported by Debian).

FreeBSD works as well (see the port mentioned above).

Our CI pipelines also build on MacOS (using brew) and Windows (using
MSYS, MINGW32, MINGW64 and Cygwin).

Somebody also did at least one successful build on Solaris (see note below).

### DEPENDENCIES

Please understand that we can't list all basic package names (like C
compilers, shells, `base-devel` etc.) for all possible distributions
and systems in this place.

Important additional dependencies for building are:

- Debian based Linux: `libpulse-dev` or `libasound2-dev`
- FreeBSD: `devel/gmake`
- Windows MINGW32: `mingw-w64-i686-toolchain`
- Windows MINGW64: `mingw-w64-x86_64-toolchain`

gbsplay can use further optional dependencies (for eg. localized
messaged, a simple GUI or additional audio drivers), see the output of
`configure` and `configure --help` for details.

If you get stuck during your build, have a peek at the build scripts
of released packages (Linux packages or FreeBSD ports) or at our CI
pipeline setup (`.github/workflow/build_*.yml` in the gbsplay
sources).

### BASIC BUILD INSTRUCTIONS

Run `make`.  This should autoconfigure and build all files.

Run `make install` (as root) to install everything to your system.
Run `make uninstall` (as root) to remove it afterwards.

### CUSTOMIZING THE INSTALLATION TARGET

To change the installation target, run the configure script manually
and pass some parameters (see `./configure --help` for a list):

```sh
./configure --prefix=/tmp/GBSPLAY
make
make install
```

Remember to use the same prefix on uninstall!

### NOTES ON FREEBSD

Be aware that you need a GNU make: You need to run `gmake` instead of `make`.

### NOTES ON SOLARIS

Solaris `/bin/sh` is not POSIX compatible (`${foo#bar}` and `${foo%bar}` are
not supported), please change the first line of configure to from `/bin/sh`
to `/usr/bin/ksh`.  The `Makefile` will need `bash` anyway (`SHELL := bash`).
