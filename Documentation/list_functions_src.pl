#! /usr/bin/perl -w

use strict;

if (!open(IN, "< ../include/vstr-extern.h"))
  { 
    die "open: $!\n"; 
  }

$/ = undef;

$_ = <IN>;

# Not valid for everything C, but good enough...

while (/^#define\s+(VSTR_[0-9a-zA-Z][0-9a-zA-Z_]*)\s*\(/gm)
  {
    print "$1()\n";
  }

while (/^extern[\sa-zA-Z_*]*\s+\**(vstr_[0-9a-zA-Z][0-9a-zA-Z_]*)\s*\(/gm)
  {
    print "$1()\n";
  }
