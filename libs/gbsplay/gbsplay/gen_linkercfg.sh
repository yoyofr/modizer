#!/bin/sh
#
# gbsplay is a Gameboy sound player
#
# 2015 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
# Licensed under GNU GPL v1 or, at your option, any later version.
#

set -eu

in="$1"
out="$2"
fmt="$3"

exec 1>"$out"

if [ "$fmt" = "ver" ]; then
	echo "{"
	echo "  global:"
else
	echo "EXPORTS"
fi

while read name; do
	if [ "$fmt" = "ver" ]; then
		echo "    ${name};"
	else
		echo "	${name}"
	fi
done < "$in"

if [ "$fmt" = "ver" ]; then
	echo "  local:"
	echo "    *;"
	echo "};"
fi
