#! /bin/sh

# export CC=gcc3

rm -f config.cache
./configure --enable-debug --with-fmt-float=glibc $@ && make clean && make check
cd Documentation/ && ./txt2html.pl && ./txt2man.pl
