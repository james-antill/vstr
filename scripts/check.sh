#! /bin/sh

C_ls=--enable-linker-script
C_dbg=--enable-debug
C_np=--enable-noposix-host
C_nin=--enable-tst-noinline
C_dbl_g=--with-fmt-float=glibc
C_dbl_h=--with-fmt-float=host
C_dbl_n=--with-fmt-float=none

function conf()
{
 echo >> autocheck1.log
 echo >> autocheck1.log
 echo "==== BEG: CC=$CC CFLAGS=$CFLAGS $@ ====" >> autocheck1.log
 echo >> autocheck1.log

 echo >> autocheck2.log
 echo "==== BEG: CC=$CC CFLAGS=$CFLAGS $@ ====" >> autocheck2.log

 ./configure $@ \
	>> autocheck1.log 2>> autocheck2.log && \
 make clean \
	>> autocheck1.log 2>> autocheck2.log && \
 make check \
	>> autocheck1.log 2>> autocheck2.log && \
 ( ./scripts/autovalgrind.sh | egrep -C 2 "^==" ) \
	>> autocheck2.log || \
 echo "==== END: CC=$CC CFLAGS=$CFLAGS $@ ====" >> autocheck1.log && \
 echo "==== END: CC=$CC CFLAGS=$CFLAGS $@ ====" >> autocheck2.log
}

function do_conf()
{
# Weird layout ... as we do normal, then nothing, hopefully spots errors quicker
 conf $1 $2
 conf $1 $2 $C_nin $C_dbl_n $C_np
 conf $1 $2 $C_np
 conf $1 $2 $C_dbl_g
 conf $1 $2 $C_dbl_n
 conf $1 $2 $C_dbl_g $C_np
 conf $1 $2 $C_dbl_n $C_np
 conf $1 $2 $C_nin 
 conf $1 $2 $C_nin $C_np
 conf $1 $2 $C_nin $C_dbl_g
 conf $1 $2 $C_nin $C_dbl_n
 conf $1 $2 $C_nin $C_dbl_g $C_np
}

if [ "x$1" = "x" ]; then
  rm -f autocheck1.log
  rm -f autocheck2.log
fi

touch autocheck1.log
touch autocheck2.log


do_conf $C_ls
do_conf $C_ls $C_dbg

export CFLAGS=$OPD_CF
do_conf $C_ls

export CFLAGS=$RPM_CF
do_conf $C_ls


