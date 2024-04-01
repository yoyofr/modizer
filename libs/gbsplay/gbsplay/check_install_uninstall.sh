#!/usr/bin/env bash
#
# Ensure that the Makefile targets 'install' and 'uninstall' work.
#
# 2020 (C) by Christian Garbs <mitch@cgarbs.de>
# Licensed under GNU GPL v1 or, at your option, any later version.

set -e

initial_files=/tmp/initial_files
files_after_installation=/tmp/files_after_installation
files_after_removal=/tmp/files_after_removal

list_files()
{
    if [ ! -d "$installdir" ]; then
	return
    fi
    
    # ignore mime database in listing:
    #   on install, files might get created for the first time,
    #   but on uninstall only file contents are removed, not the files themselves
    #
    # also ignore $installdir which might remain because of the remaining mime database files
    find "$installdir" \
	| sort \
	| grep -v -E '/share$' \
	| grep -v -E '/share/applications$' \
	| grep -v -E '/share/applications/mimeinfo\.cache' \
	| grep -v -E '/share/mime$' \
	| grep -v -E '/share/mime/(aliases|audio|generic-icons|globs|globs2|icons|magic|mime\.cache|subclasses|treemagic|types|version|XMLnamespaces)$' \
	| grep -v -E '/share/mime/application$' \
	| grep -v -E '/share/mime/application/x-cmakecache\.xml$' \
	| grep -v -E "^$installdir$" || true
}

expect_content_to_be_equal()
{
    local file_a="$1" file_b="$2"

    printf 'compare <%s> to <%s> .. ' "$file_a" "$file_b"
    if ! cmp --quiet "$file_a" "$file_b"; then
	echo "ERROR"
	echo "expected content of <$file_a> and <$file_b> to be equal, but file contents differ:"
	diff "$file_a" "$file_b"
	exit 1
    fi
    echo "ok"
}

expect_content_to_be_different()
{
    local file_a="$1" file_b="$2"

    printf 'compare <%s> to <%s> .. ' "$file_a" "$file_b"
    if cmp --quiet "$file_a" "$file_b"; then
	echo "ERROR"
	echo "expected content of <$file_a> and <$file_b> to be different, but file contents are equal"
	exit 1
    fi
    echo "ok"
}

## read installation directory from configuration

read -r _ _ installdir < <(grep "^prefix " config.mk)

## run install/uninstall cycle

list_files > "$initial_files"
make install
list_files > "$files_after_installation"
make uninstall
list_files > "$files_after_removal"

## check file changes

echo
expect_content_to_be_different "$initial_files"            "$files_after_installation"
expect_content_to_be_different "$files_after_installation" "$files_after_removal"
expect_content_to_be_equal     "$initial_files"            "$files_after_removal"

## end
echo ok
