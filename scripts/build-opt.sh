#! /bin/sh

CFLAGS="-O2 -march=i386 -mcpu=i686" ./configure --enable-linker-script && \
 make clean && make check
