#!/usr/bin/env bash
#
# Ensure that configure enabled the plugouts as it was expected to.
#
# 2020-2022 (C) by Christian Garbs <mitch@cgarbs.de>
# Licensed under GNU GPL v1 or, at your option, any later version.

set -e

die()
{
    echo "ERROR"
    echo "$@"
    echo
    echo "dumping config.mk:"
    cat config.mk
    exit 1
}

is_contained_in()
{
    local searched="$1" current
    shift
    for current in "$@"; do
	if [ "$current" = "$searched" ]; then
	    return 0
	fi
    done
    return 1
}

# these don't have dependencies and should always be built
declare -a expected_plugouts=( iodumper midi altmidi stdout vgm wav )

# additional plugouts are given as commandline parameters
expected_plugouts+=( $* )

# read configured plugouts
declare -a enabled_plugouts

while read -r prefixed_name _ state; do
    name=${prefixed_name:8}
    if [ "$state" = yes ]; then
	enabled_plugouts+=( $name )
    fi
done < <( grep ^plugout config.mk)

# dump status
echo "expected plugouts: ${expected_plugouts[@]}"
echo "checking config.mk"

# check that all expected plugouts are present
for expected in "${expected_plugouts[@]}"; do
    printf 'check expected <%s> to be enabled .. ' "$expected"
    if ! is_contained_in "$expected" "${enabled_plugouts[@]}"; then
	die "expected plugout <$expected> has not been enabled by configure"
    fi
    echo "ok"
done

# check that no unexpected plugouts are present
for enabled in "${enabled_plugouts[@]}"; do
    printf 'check enabled <%s> to be expected .. ' "$enabled"
    if ! is_contained_in "$enabled" "${expected_plugouts[@]}" ; then
	die "unexpected plugout <$enabled> has been enabled by configure"
    fi
    echo "ok"
done

# ok
echo "ok"
