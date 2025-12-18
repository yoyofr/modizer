#!/bin/sh

./autoclean.sh

rm -f configure

rm -f Makefile.in

rm -f config.guess
rm -f config.sub
rm -f install-sh
rm -f missing
rm -f depcomp

if [ 0 = 1 ]; then
autoscan
else
#cd pflib && ./autogen.sh && cd ..

touch NEWS
touch README
touch AUTHORS
touch ChangeLog
touch config.h.in

libtoolize --copy --force
aclocal
automake -a -c
autoconf

#./configure --enable-debug --enable-fmtwmmdic --enable-fmtpowerword --enable-fmtbabylonbdc --without-icu4c
#./configure --enable-debug --enable-fmtwmmdic --enable-fmtpowerword --enable-fmtbabylonbdc
./configure --enable-debug --with-config=ultimate
make clean
make

fi
