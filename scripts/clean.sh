#! /bin/sh

if false; then
 echo "Not reached."
elif [ -f ./configure ]; then
        r=./
elif [ -f ../configure ]; then
        r=../
else
  echo "Not in right place, dying."
  exit 1;
fi

cd $r
make distclean 2>&1 > /dev/null

rm -rf tst/*/.libs

rm -rf autocheck?.log autom4te.cache
rm -f  include/autoconf.h include/stamp-h include/vstr-conf.h src/vstr_version.c
rm -f  gmon.out examples/gmon.out
rm -f  Documentation/vstr.3 Documentation/vstr_const.3

cd examples; make clean 2>&1 > /dev/null

