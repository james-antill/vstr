#! /bin/sh

if [ "x$CC" = "x" ]; then
 CC=gcc
fi

export expot CFLAGS="-DUSE_RESTRICTED_HEADERS -O2 -march=i386 -mcpu=i686"
export CC="diet $CC"
rm -f config.cache
./configure --disable-shared --enable-linker-script $@ && \
 make clean && make check
