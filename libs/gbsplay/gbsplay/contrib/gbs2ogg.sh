#!/bin/sh
#
# Automatically convert all subsongs from .gbs file to .ogg files.
# 
# 2003-2005,2008,2019 (C) by Christian Garbs <mitch@cgarbs.de>
#
# Licensed under GNU GPL v1 or, at your option, any later version
#

FILENAME=$1

QUALITY=6
RATE=44100
PLAY=150
FADE=6
GAP=0

if [ -z "$1" ]; then
    echo "usage:  gbs2ogg.sh <gbs-file> [subsong_seconds]"
    exit 1
fi

if [ "$2" ]; then
    PLAY=$2
fi

FILEBASE=$(basename "$FILENAME")
FILEBASE=$(echo "$FILEBASE"|sed 's/.gbs$//')

# get subsong count
# get song info
gbsinfo "$FILENAME" | cut -d : -f 2- | sed -e 's/^ *//' -e 's/^"//' -e 's/"$//' | (
    read -r _ #GBSVERSION
    read -r TITLE
    read -r AUTHOR
    read -r COPYRIGHT
    read -r _ #LOAD_ADDR
    read -r _ #INIT_ADDR
    read -r _ #PLAY_ADDR
    read -r _ #STACK_PTR
    read -r _ #FILE_SIZE
    read -r _ #ROM_SIZE
    read -r SUBSONGS

    for SUBSONG in $(seq 1 "$SUBSONGS"); do
	OGGFILE="$(printf "%s-%02d.ogg" "$FILEBASE" "$SUBSONG")"
	printf "== converting song %02d/%02d:\n" "$SUBSONG" "$SUBSONGS"
	gbsplay -o stdout -E l -r "$RATE" -g "$GAP" -f "$FADE" -t "$PLAY" "$FILENAME" "$SUBSONG" "$SUBSONG" </dev/null \
	    | oggenc -q"$QUALITY" -r --raw-endianness 0 -B 16 -C 2 -R "$RATE" \
		     -N "$SUBSONG" -t "$TITLE" -a "$AUTHOR" -c "copyright=$COPYRIGHT" -c "COMMENT=Subsong $SUBSONG" -G "Gameboy music" \
		     -o "$OGGFILE" -
    done
)
