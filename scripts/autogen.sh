#! /bin/sh

aclocal && automake && autoconf

# Gather internal symbols...
rm -f include/vstr-internal-alias-symbols.h
touch include/vstr-internal-alias-symbols.h
rm -f include/vstr-internal-cpp-symbols.h
touch include/vstr-internal-cpp-symbols.h

rm -f include/vstr-internal-alias-inline-symbols.h
touch include/vstr-internal-alias-inline-symbols.h
rm -f include/vstr-internal-cpp-inline-symbols.h
touch include/vstr-internal-cpp-inline-symbols.h



# Do the inline helpers by hand...
cat include/vstr-internal.export_symbols | while read i; do
 echo "VSTR__SYM($i)" >> include/vstr-internal-alias-symbols.h
 echo "#define vstr_nx_$i vstr_$i" >> include/vstr-internal-cpp-symbols.h
done
cat include/vstr-internal.inline_export_symbols | while read i; do
 echo "VSTR__SYM($i)" >> include/vstr-internal-alias-inline-symbols.h
 echo "#define vstr_nx_$i vstr_$i" >> include/vstr-internal-cpp-inline-symbols.h
done

# produce a local and external copy of vstr-extern.h ...

perl -pe 'BEGIN { print "/* DO NOT EDIT THIS FILE */\n"; } \
          s/\@{4}vstr_/vstr_$1/; \
          s/\@{4};/;/;' \
  include/vstr-extern.h.pre > include/vstr-extern.h
perl -pe 'BEGIN { print "/* DO NOT EDIT THIS FILE */\n"; } \
          s/\@{4}vstr_/vstr_nx_$1/; \
          s/\@{4};/VSTR__ATTR_H() ;/;' \
  include/vstr-extern.h.pre > include/vstr-internal-extern.h

# produce a local and external copy of vstr-inline.h ...

perl -pe 'BEGIN { print "/* DO NOT EDIT THIS FILE */\n"; } \
          s/\@{4}vstr_(extern_inline_)/vstr_$1/; \
          s/\@{4}vstr_/vstr_/;' \
  include/vstr-inline.h.pre > include/vstr-inline.h
perl -pe 'BEGIN { print "/* DO NOT EDIT THIS FILE */\n"; } \
          s/\@{4}vstr_(extern_inline_)/vstr_nx_$1/; \
          s/\@{4}vstr_/vstr_nx_/;' \
  include/vstr-inline.h.pre > include/vstr-internal-inc-inline.h

# perl -pe 'BEGIN { print "/* DO NOT EDIT THIS FILE */\n"; } \
#           s/\@{4}vstr_(extern_inline_)/vstr_nx_$1/; \
#           s/\@{4}vstr_/vstr_/;' \
#   include/vstr-inline.h.pre > include/vstr-internal-c-inline.h


