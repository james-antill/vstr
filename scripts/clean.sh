#! /bin/sh

make distclean 2>&1 > /dev/null

rm -rf autocheck?.log autom4te-2.53.cache
rm -f  include/autoconf.h include/vstr-conf.h
rm -f  gmon.out examples/gmon.out Documentation/vstr.3

cd examples; make clean
