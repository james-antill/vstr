#! /bin/sh

./configure --enable-debug --enable-linker-script && make clean && make check
