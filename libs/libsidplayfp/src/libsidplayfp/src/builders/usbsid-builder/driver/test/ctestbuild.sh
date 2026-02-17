#!/usr/bin/env bash

LIBDIR=$(pwd)/../libusbsiddrv-vice

rm -f *.o gcc-test

g++ --verbose $LIBDIR/*.cpp c-test.c \
  -fPIC -Wshadow -Wconversion -Wall -Werror \
  -I$LIBDIR -L$LIBDIR \
  $(pkg-config --cflags --libs libusb-1.0) \
  -o gcc-test ;

rm -f *.o gcc-test

# rm -f *.o clang-test

# clang --verbose -stdlib=libc++ $LIBDIR/*.cpp c-test.c \
#   -fPIC -Wshadow -Wconversion -Wall -Werror -Wc99-designator -Wshorten-64-to-32
#   -I$LIBDIR -L$LIBDIR \
#   $(pkg-config --cflags --libs libusb-1.0) \
#   -o clang-test
