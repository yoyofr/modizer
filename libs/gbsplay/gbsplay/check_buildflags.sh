#!/usr/bin/env bash
#
# Ensure that configure enabled the plugouts as it was expected to.
#
# 2020-2021 (C) by Christian Garbs <mitch@cgarbs.de>
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

key_contains()
{
    local key="$1" searched="$2" 

    local -a values
    read -a values -r < <(grep "^$key := " config.mk)

    local value
    # :2 skips the first two array entries ($key and ":=")
    for value in "${values[@]:2}"; do
	if [ "$value" = "$searched" ]; then
	    return 0
	fi
    done
    return 1
}

expect_to_contain()
{
    local key="$1" expected="$2"
    
    printf 'check <%s> to contain <%s> .. ' "$key" "$expected"
    if ! key_contains "$key" "$expected"; then
	die "expected value <$expected> not found in key <$key>"
    fi
    echo "ok"
}

expect_to_not_contain()
{
    local key="$1" unexpected="$2"

    printf 'check <%s> to not contain <%s> .. ' "$key" "$unexpected"
    if key_contains "$key" "$unexpected"; then
	die "unexpected value <$unexpected> found in key <$key>"
    fi
    echo "ok"
}

# default configuration when all dependencies are available
i18n=yes
zlib=yes
harden=yes
sharedlib=no
verbosebuild=no
xgbsplay=no
prefix=/usr/local

# parse build configuration
for flag in "$@"; do
    case "$flag" in
	"")
	;;
	--disable-i18n)
	    i18n=no
	    ;;
	--disable-zlib)
	    zlib=no
	    ;;
	--disable-hardening)
	    harden=no
	    ;;
	--enable-sharedlibgbs)
	    sharedlib=yes
	    ;;
	--enable-verbosebuild)
	    verbosebuild=yes
	    ;;
	--with-xgbsplay)
	    xgbsplay=yes
	    ;;
	--prefix=*)
	    prefix="${flag#--prefix=}"
	    ;;
	*)
	    die "unhandled flag <$flag>"
	    ;;
    esac
done

# dump status
echo "expected configuration: i18n=$i18n zlib=$zlib harden=$harden sharedlib=$sharedlib"
echo "checking config.mk"

# test configuration
expect_to_contain use_i18n "$i18n"

if [ "$zlib" = yes ]; then
    expect_to_contain EXTRA_LDFLAGS "-lz"
else
    expect_to_not_contain EXTRA_LDFLAGS "-lz"
fi

if [ "$harden" = yes ]; then
    expect_to_contain EXTRA_CFLAGS "-fstack-protector-strong"
    expect_to_contain EXTRA_LDFLAGS "-fstack-protector-strong"
else
    expect_to_not_contain EXTRA_CFLAGS "-fstack-protector"
    expect_to_not_contain EXTRA_CFLAGS "-fstack-protector-strong"
    expect_to_not_contain EXTRA_CFLAGS "-fstack-clash-protection"
    expect_to_not_contain EXTRA_LDFLAGS "-fstack-protector"
    expect_to_not_contain EXTRA_LDFLAGS "-fstack-protector-strong"
    expect_to_not_contain EXTRA_LDFLAGS "-fstack-clash-protection"
fi

expect_to_contain use_sharedlibgbs "$sharedlib"

expect_to_contain use_verbosebuild "$verbosebuild"

if [ "$xgbsplay" = yes ]; then
    expect_to_contain EXTRA_INSTALL "xgbsplay"
    expect_to_contain EXTRA_UNINSTALL "xgbsplay"
else
    expect_to_not_contain EXTRA_INSTALL "xgbsplay"
    expect_to_not_contain EXTRA_UNINSTALL "xgbsplay"
fi

expect_to_contain prefix "$prefix"

# ok
echo "ok"
