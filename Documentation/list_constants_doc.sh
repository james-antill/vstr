#! /bin/sh

egrep "^Constant" constants.txt | awk '{ print $2 }'
