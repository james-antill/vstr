#! /bin/sh

if false; then
 echo "Not reached."
elif [ -f ./configure ]; then
        s=./scripts
	doln=false
elif [ -f ../configure ]; then
        s=../scripts
	doln=true
else
  echo "Not in right place, dying."
  exit 1;
fi

$s/b/def-coverage.sh
# ./src/.libs/libvstr-*.so.*

cd src

if $doln; then
 lndir ../$s/../src
fi

ggcov

