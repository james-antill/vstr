#! /bin/sh

rm -f config.cache
./configure --enable-debug $@ && make clean && make check
