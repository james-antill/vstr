#! /bin/sh

export CFLAGS="-DUSE_RESTRICTED_HEADERS"
export CC="diet gcc"
rm -f config.cache
./configure --disable-shared --enable-debug --enable-linker-script $@ && \
  make clean && make check
