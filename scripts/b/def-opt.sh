#! /bin/sh

if false; then
 echo "Not reached."
elif [ -f ./configure ]; then
        c=./configure
elif [ -f ../configure ]; then
        c=../configure
else
  echo "Not in right place, dying."
  exit 1;
fi


CFLAGS="-O2 -march=i386 -mcpu=i686" \
  $c --enable-wrap-memset --enable-wrap-memcpy --enable-linker-script \
    --with-fmt-float=glibc $@ && \
  make clean && make check
