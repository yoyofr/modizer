sidplayfp
=========

https://github.com/libsidplayfp/sidplayfp

sidplayfp is a console C64 music player which uses the libsidplayfp engine
to provide the best SID listening experience.

Copyright (c) 2000 Simon White

Copyright (c) 2007-2010 Antti Lankila

Copyright (c) 2010-2025 Leandro Nini (drfiemost@users.sourceforge.net)


stilview
========

STILView is a command-line driven program to help you retrieve
the entries stored in STIL fast and accurately.

Copyright (c) 1998, 2002 LaLa

Copyright (c) 2013-2017 Leandro Nini (drfiemost@users.sourceforge.net)

-----------------------------------------------------------------------------

_This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version._

_This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details._

_You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA_

-----------------------------------------------------------------------------

Documentation
-------------
https://github.com/libsidplayfp/sidplayfp/wiki

-----------------------------------------------------------------------------

NOTE:
-----
ROM dumps are not embedded and must be supplied by the user.
These ROMs are optional and most tunes should work fine without them,
but compatibility is not guaranteed.
Check `sidplayfp.ini`'s documentation for configuration details
and default search paths.

-----------------------------------------------------------------------------

## Build

This package uses autotools, so the usual `./configure && make` is enough to build
the programs. If cloning the bare sources, the package needs to be bootstrapped
in advance with `autoreconf -vfi`.

In addition to the standard build options the following are available:

`--enable-debug`:
compile with debugging messages,
disabled by default

`--with-libiconv-prefix[=DIR]`:
search for libiconv in DIR/include and DIR/lib

the character conversion requires presence of the POSIX/XSI iconv function family in either the C library or a separate libiconv library (see https://www.gnu.org/software/gettext/manual/html_node/AM_005fICONV.html)

-----------------------------------------------------------------------------

# 3rd party software

* miniaudio
  https://github.com/mackron/miniaudio
  distributed under MIT license
