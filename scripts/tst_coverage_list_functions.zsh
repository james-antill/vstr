#! /bin/zsh

for i in tst/**/.libs/tst_*; do

 nm -u "$i" | egrep "^vstr_" | sed -e 's/$/()/;'

done | sort | uniq

