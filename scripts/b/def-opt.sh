#! /bin/sh

if false; then
 echo "Not reached."
elif [ -f ./configure ]; then
        c=./configure
        D=Documentation/
elif [ -f ../configure ]; then
        c=../configure
        D=../Documentation/
else
  echo "Not in right place, dying."
  exit 1;
fi


CFLAGS="-O2 -march=i386 -mcpu=i686" \
  $c --with-fmt-float=glibc --enable-linker-script $@ && \
 make clean && make check && \
cd $D && ./txt2html.pl && ./txt2man.pl

