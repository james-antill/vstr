#! /bin/sh

if [ "x$CC" = "x" ]; then
 CC=gcc
fi

export CFLAGS="-DUSE_RESTRICTED_HEADERS -g3"
export CC="diet $CC"
rm -f config.cache
./configure --disable-shared --enable-debug --enable-linker-script $@ && \
  make clean && make check
