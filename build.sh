#!/bin/sh

mkdir -p `pwd`/../cilk-plus-rts
libtoolize
aclocal
automake --add-missing
autoconf
./configure --prefix=`pwd`/../cilk-plus-rts
make -j
make install
