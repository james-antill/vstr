#! /bin/sh -e

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


# Remove ccache... 
if [ "x$CC" = "x" ]; then
for i in `perl -e 'print join "\n", split ":", $ENV{PATH}'`; do
  if [ "x$CC" = "x" ]; then
    if [ -f $i/gcc ]; then
      if ! readlink $i/gcc | egrep -q ccache; then
        export CC=$i/gcc
      fi
    fi
  fi
done
fi


# only remove the known .info files once...
rm -f lib-dbg.info lib-opt.info tst-dbg.info tst-opt.info

function del()
{
    rm -rf Documentation/ include/ src/ tst/ \
    Makefile config.log config.status libtool vstr.pc vstr.spec
}

function linkup()
{
  cd src

  if $doln; then
   lndir ../$s/../src
  fi

  for i in .libs/*.da; do
    ln -f $i; rm -f $i
  done

  cd ..
}

function cov()
{
  type=$1; shift
  del
  $s/b/COVERAGE.sh $@
  linkup
  $s/lcov.sh $type
}

cov dbg --enable-debug
cov opt
cov noopt --enable-tst-noopt
cov ansi --enable-noposix-host \
         --enable-tst-noattr-visibility \
         --enable-tst-noattr-alias \
         --enable-tst-nosyscall-asm
