#! /bin/sh

if false; then
 echo "Not reached."
# elif [ -f ./configure ]; then
        s=./scripts
        s=./Documentation
	doln=false
elif [ -f ../configure ]; then
        s=../scripts
        d=../Documentation
	doln=true
else
  echo "Not in right place, goto a seperate build directory."
  exit 1;
fi

if [ "x$1" = "x" ]; then
  echo "Not got arg."
  exit 1;
fi

gendesc $d/coverage_descriptions.txt -o descriptions

cd src
lcov --capture --directory . --output-file ../lib-$1.info --test-name lib$1
cd ..

# cd tst
# lcov --capture --directory . --output-file ../tst-$1.info --test-name tst-$1
# cd ..

genhtml *.info --output-directory output --title "Vstr coverage" --show-details --description-file $d/tst-descriptions
echo Point your browser at file:`pwd`/output/index.html
