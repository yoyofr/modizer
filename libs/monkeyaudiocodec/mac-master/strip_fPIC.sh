#!/bin/sh
#
# libtool assumes that the compiler can handle the -fPIC flag
# This isn't always true (for example, nasm can't handle it)
command=""
while [ $1 ]; do
	if [ "$1" != "-fPIC" ]; then
		if [ "$1" != "-DPIC" ]; then
			if [ "$1" != "-fno-common" ]; then
				command="$command $1"
			fi
		fi
	fi
	shift
done
echo $command
exec $command
