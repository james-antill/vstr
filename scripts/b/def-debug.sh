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

$c --enable-debug --with-fmt-float=glibc $@ && make clean && make check && \
cd $D && ./txt2html.pl && ./txt2man.pl
