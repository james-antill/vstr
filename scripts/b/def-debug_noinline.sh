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

args="--enable-debug --with-fmt-float=glibc --enable-tst-noinline"

$c $args $@ && make clean && make check
