#! /bin/sh

if false; then
 echo "Not reached."
elif [ -f ./configure ]; then
        c=./configure
elif [ -f ../configure ]; then
        c=../configure
else
  echo "Not in right place, dying."
  exit 1;
fi

# float=--with-fmt-float=glibc
float=--with-fmt-float=host

export CFLAGS="-g -fprofile-arcs -ftest-coverage -DUSE_SYSCALL_MAIN"
$c \
  --enable-tst-noinline \
  --enable-tst-nosyscall-asm \
  --enable-tst-noassert-loop \
  --enable-tst-noattr-alias \
  --enable-tst-noopt \
  --enable-debug \
  --enable-wrap-memcpy \
  --enable-wrap-memcmp \
  --enable-wrap-memchr \
  --enable-wrap-memrchr \
  --enable-wrap-memset \
  --enable-wrap-memmove \
  $float \
    $@ && make clean && make check
