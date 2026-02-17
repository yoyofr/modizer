libresidfp
==========

Cycle exact SID emulation.

This project is meant to replicate the SID as faithfully as possible
while keeping good performance for realtime use.
It is not intended to expose the chip internal state or adding fancy effects.
Both the 6581 and the 8580 models are emulated.

https://github.com/libsidplayfp/libresidfp

Copyright (c) 2000-2011 Dag Lem  
Copyright (c) 2007-2010 Antti Lankila  
Copyright (c) 2010-2026 Leandro Nini (drfiemost@users.sourceforge.net)

_Warning!_ still experimental and subject to change, use at your own risk

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

## API docs

https://libsidplayfp.github.io/libresidfp/


## Build

This package uses autotools, so the usual `./configure && make` is enough to build
the libraries. If cloning the bare sources the package needs to be bootstrapped
in advance with the `autoreconf -vfi` command.

In addition to the standard build options the following are available:

* `--enable-debug[=no/yes]`:
compile for debugging with inlining disabled and warnings
(disabled by default)

* `--enable-branch-hints`:
enable branch hints in the reSID engine so the compiler can produce more optimized code
(enabled by default)

* `--with-simd=<runtime/mmx/sse2/sse4/avx2/avx512f/none>`:
enable x86 SIMD code for resampling.
Not required if `-march` or `-mcpu` is already included in the compiler flags
(i.e. `CXXFLAGS=-march=x86-64-v3`).
_runtime_ enables runtime dispatch of the resampling function depending on the CPU
supported instruction set. Works only when compiling with gcc.
(none by default)

* `--enable-lto`:
enable Link Time Optimization if supported by compiler
(disabled by default)

* `--disable-tests`:
disable unit tests and test programs
(enabled by default)
