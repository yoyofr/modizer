#!/bin/sh
#
# gbsplay is a Gameboy sound player
#
# 2003-2004,2019 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
#                       Christian Garbs <mitch@cgarbs.de>
# Licensed under GNU GPL v1 or, at your option, any later version.
#

DIR=$(dirname "$1")
FILE="$1"
SUBMK=""

shift

EXTRADEP=" $*"

if [ "$DIR" = "" ] || [ "$DIR" = "." ]; then
	DIR=""
else
	DIR="$DIR/"
fi
if [ -f "${DIR}subdir.mk" ]; then
	SUBMK=" ${DIR}subdir.mk"
fi

exec "$CC" -M $GBSCFLAGS "$FILE" |
	sed -n -e "
		s@^\\(.*\\)\\.o:@$DIR\\1.d $DIR\\1.o $DIR\\1.lo: depend.sh Makefile$SUBMK$EXTRADEP@
		s@/usr/[^	 ]*@@g
		t foo
		:foo
		s@^[	 ]*\\\\\$@@
		t
		p
		"
