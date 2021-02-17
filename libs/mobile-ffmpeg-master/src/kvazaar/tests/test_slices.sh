#!/bin/sh

set -eu
. "${0%/*}/util.sh"

valgrind_test 512x256 10 --threads=2 --owf=1 --preset=ultrafast --tiles=2x2 --slices=tiles
valgrind_test 264x130 10 --threads=2 --owf=1 --preset=ultrafast --slices=wpp
if [ ! -z ${GITLAB_CI+x} ];then valgrind_test 264x130 20 --threads=2 --owf=1 --preset=fast --slices=wpp --no-open-gop; fi
