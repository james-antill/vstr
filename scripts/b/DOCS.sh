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

$c --enable-debug --enable-tst-noattr-visibility --enable-tst-noattr-alias --enable-wrap-memcpy--enable-wrap-memcmp --enable-wrap-memchr --enable-wrap-memrchr --enable-wrap-memset --enable-wrap-memmove --with-fmt-float=glibc $@ && make clean && make check
