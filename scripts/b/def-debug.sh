#! /bin/sh

rm -f config.cache
./configure --enable-debug $@ && make clean && make check
cd Documentation/ && ./txt2html.pl && ./txt2man.pl
