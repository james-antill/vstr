#! /usr/bin/perl -w

use strict;

if (!open(IN, "< ../include/vstr-const.h"))
  { 
    die "open: $!\n"; 
  }

$/ = undef;

$_ = <IN>;
while (/^#define\s+(VSTR_[0-9a-zA-Z][0-9a-zA-Z_]*)\s/gm)
  {
    print "$1\n";
  }

if (!open(IN, "< ../include/vstr-switch.h"))
  { 
    die "open: $!\n"; 
  }

$_ = <IN>;
while (/^#\s*define\s+(VSTR_COMPILE_[0-9a-zA-Z][0-9a-zA-Z_]*)\s/gm)
  {
    print "$1\n";
  }

