#! /bin/sh

CFLAGS="-O2 -march=i386 -mcpu=i686 -pg" ./configure --enable-linker-script && \
 make clean && make check
