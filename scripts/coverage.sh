#! /bin/sh -e

tst_klibc=false

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
  for dir in src examples; do
  cd $dir

  if $doln; then
   lndir ../$s/../$dir
  fi

# Newer GCCs put them in the $srcdir
  if [ ! -f ex_highlight-ex_highlight.da -a ! -f vstr.da ]; then
    for i in .libs/*.da; do
      ln -f $i; rm -f $i
    done
  fi

  cd ..
  done
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

CFLAGS="-DUSE_RESTRICTED_HEADERS=1" cov small-libc

# This doesn't work, coverage with either klibc or dietlibc are NOGO...
# However you can compile programs linked with either and Vstr ... just not with
# coverage.
if $tst_klibc; then
  export LDFLAGS="-nodefaultlibs /usr/lib/klibc/crt0.o `gcc --print-libgcc` /usr/lib/klibc/libc.a"
  export CFLAGS="-nostdinc -iwithprefix include -I/usr/include/klibc -I/usr/include/klibc/kernel"
  cov klibc
fi

