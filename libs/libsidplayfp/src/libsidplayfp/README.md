libsidplayfp
============

https://github.com/libsidplayfp/libsidplayfp

libsidplayfp is a C64 music player library which integrates
the reSIDfp SID chip emulation engine into a cycle-based emulator
environment, constantly aiming to improve emulation of the
C64 system and the SID chips.

Copyright (c) 2000-2001 Simon White  
Copyright (c) 2007-2010 Antti Lankila  
Copyright (c) 2010-2026 Leandro Nini (drfiemost@users.sourceforge.net)

-----------------------------------------------------------------------------

_This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version._

_This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details._

_You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA._

-----------------------------------------------------------------------------

**NOTE**: the git repository contains submodules,
          clone with `--recurse-submodules`.

-----------------------------------------------------------------------------

API documentation available online at

https://libsidplayfp.github.io/libsidplayfp/

-----------------------------------------------------------------------------

## Build

This package uses autotools, so the usual `./configure && make` is enough to build
the libraries. If cloning the bare sources the package needs to be bootstrapped
in advance with the `autoreconf -vfi` command. This step requires the xa cross-assembler
to compile the 6502 code, usually provided by distributions with the xa65 package
or available at http://www.floodgap.com/retrotech/xa/.
To enable full quality SID emulation [libresidfp](https://github.com/libsidplayfp/libresidfp)
is required.

In addition to the standard build options the following are available:

* `--enable-debug[=no/yes/full]`:
compile with debugging messages
(disabled by default)

* `--enable-testsuite=PATH_TO_TESTSUITE`:
add support for running VICE's testsuite (in PRG format). The testsuite is available
in the repository. Intended only for regression tests since it may break normal
code execution. The path to testsuite must include terminal path separator.
(disabled by default)

* `--enable-tests`:
enables unit tests. Use `make check` to launch the testsuite
(disabled by default)

* `--with-exsid`:
Build with exsid support. Requires either libexsid or one of libfdti1 or libftd2xx

* `--with-usbsid`:
Build with usbsid support. Requires either libusb


If [doxygen](https://doxygen.nl) is installed and detected by the configure script, the documentation
can be built by invoking `make doc`.



Known bugs/limitations:
* mus data embedded in psid file is not supported

-----------------------------------------------------------------------------

# 3rd party software

* hashlib
  https://github.com/Cra3z/hashlib
  distributed under MIT license
