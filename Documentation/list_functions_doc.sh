#! /bin/sh

egrep "^Function" functions.txt | awk '{ print $2 }'
