#! /bin/sh

if false; then
 echo "Not reached."
# elif [ -f ./configure ]; then
        s=./scripts
	doln=false
elif [ -f ../configure ]; then
        s=../scripts
	doln=true
else
  echo "Not in right place, goto a seperate build directory."
  exit 1;
fi

$s/b/COVERAGE.sh

cd src

if $doln; then
 lndir ../$s/../src
fi

cd ..
$s/ggcov.sh

