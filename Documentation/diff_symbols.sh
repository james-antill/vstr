#! /bin/sh

(
 ./list_constants_doc.sh;
 ./list_constants_src.pl

 ./list_functions_doc.sh;
 ./list_functions_src.pl ) | \
   sort | uniq -u
