#! /usr/bin/perl -w

use strict;

push @INC, "$ENV{SRCDIR}/tst";
require 'vstr_tst_examples.pl';

run_tst("ex_hexdump");

our $opts = "--mmap";
run_tst("ex_hexdump");

$opts = "--none";
run_tst("ex_hexdump", "ex_hexdump_none");

success();
