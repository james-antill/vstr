#! /bin/sh

aclocal && automake && autoconf

# Gather internal symbols...
rm -f include/vstr-alias-symbols.h
touch include/vstr-alias-symbols.h
rm -f include/vstr-alias-inline-symbols.h
touch include/vstr-alias-inline-symbols.h
rm -f include/vstr-cpp-symbols.h
touch include/vstr-cpp-symbols.h
rm -f include/vstr-cpp-inline-symbols.h
touch include/vstr-cpp-inline-symbols.h

# Do the inline helpers by hand...
cat include/export_symbols | while read i; do
 echo "VSTR__AH($i)" >> include/vstr-alias-symbols.h
 echo "#define vstr_nx_$i vstr_$i" >> include/vstr-cpp-symbols.h
done

cat include/export_symbols_inline | while read i; do
 echo "VSTR__AH($i)" >> include/vstr-alias-inline-symbols.h
 echo "#define vstr_nx_$i vstr_$i" >> include/vstr-cpp-inline-symbols.h
done

# produce a local and external copy of inline.h ...

perl -pe 's/\@{4}vstr_(extern_inline_[_a-z]+)/vstr_$1/;' \
  include/vstr-inline.h.pre > include/vstr-inline.h
perl -pe 's/\@{4}vstr_(extern_inline_[_a-z]+)/vstr_nx_$1/;' \
  include/vstr-inline.h.pre > include/vstr-internal-inline.h
