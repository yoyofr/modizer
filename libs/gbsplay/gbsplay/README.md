# gbsplay - A Gameboy sound player

[![Code coverage status](https://codecov.io/github/mmitch/gbsplay/coverage.svg)](https://codecov.io/github/mmitch/gbsplay?branch=master)
[![Linux Build status](https://github.com/mmitch/gbsplay/workflows/Linux%20Build/badge.svg)](https://github.com/mmitch/gbsplay/actions?query=workflow%3A%22Linux+Build%22)
[![FreeBSD Build status](https://github.com/mmitch/gbsplay/workflows/FreeBSD%20Build/badge.svg)](https://github.com/mmitch/gbsplay/actions?query=workflow%3A%22FreeBSD+Build%22)
[![macOS Build status](https://github.com/mmitch/gbsplay/workflows/macOS%20Build/badge.svg)](https://github.com/mmitch/gbsplay/actions?query=workflow%3A%22macOS+Build%22)
[![Windows Build status](https://github.com/mmitch/gbsplay/workflows/Windows%20Build/badge.svg)](https://github.com/mmitch/gbsplay/actions?query=workflow%3A%22Windows+Build%22)
[![CodeQL status](https://github.com/mmitch/gbsplay/workflows/CodeQL/badge.svg)](https://github.com/mmitch/gbsplay/actions?query=workflow%3ACodeQL)

This program emulates the sound hardware of the Nintendo Gameboy.  It
is able to play the sounds from a Gameboy module dump (.GBS format, as
well as the older .GBR format), a subset of plain Gameboy rom dumps (.GB
format; button input and graphics are not supported) and Gameboy Video
Game Music dumps (.VGM/.VGZ format).

Homepage/Repo:   https://github.com/mmitch/gbsplay/
Bug reports:     please use the issue tracker at
                 https://github.com/mmitch/gbsplay/issues

gbsplay is compatible to GBSv1.  It uses a backwards compatible extension
to store additional information in the file.  The gbsplay package contains
the following parts:

 * `gbsplay`:    a console player
 * `gbsinfo`:    displays information about a gbs file

## Packaging status

[![Packaging status](https://repology.org/badge/vertical-allrepos/gbsplay.svg)](https://repology.org/project/gbsplay/versions)

## License

``
(C) 2003-2025 by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
                 Christian Garbs <mitch@cgarbs.de>
                 Maximilian Rehkopf <otakon@gmx.net>
                 Vegard Nossum <vegardno@ifi.uio.no>
                 Lisa Riedler <dev@riedler.wien>

Source Code licensed under GNU GPL v1 or, at your option, any later version.

Individual copyright notices can be found in the file headers.
The copyright notice for each file should be in the following form:

"(C) 20xx-20yy by List of File Major Contributors"

Where 20xx is the first release year for the file, 20yy is the most
recent release year for the file.

For detailed author attribution, see the revision history of each file.

Additionally, the following files are included under different licenses:

File:       examples/nightmode.gbs
Copyright:  Laxity of Dual Crew Shining (Nightmode Demo)
License:    Public Domain
Homepage:   http://www.dc-s.com/

File:       crc32.c
Copyright:  Provided to GNUnet by peter@horizon.com
License:    Public Domain
Homepage:   n/a
``
