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

gendesc ../Documentation/coverage_descriptions.txt -o descriptions

cd src
lcov --capture --directory . --output-file lib.info --test-name "lib"
cd ..

# tst_info=tst/tst.info
# cd tst
# lcov --capture --directory . --output-file tst.info --test-name "tst"
# cd ..

genhtml src/lib.info $tst_info --output-directory output --title "Vstr coverage" --show-details --description-file descriptions
echo Point your browser at file://`pwd`/output/index.html
