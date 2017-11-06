#!/bin/sh

mkdir -p ../cilk-plus-rts
libtoolize
aclocal
automake --add-missing
autoconf
./configure --prefix=../cilk-plus-rts/
make -j
make install
