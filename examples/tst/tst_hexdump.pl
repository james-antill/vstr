#! /usr/bin/perl -w

my @tsts = ("ex_hexdump");

use strict;

push @INC, "$ENV{SRCDIR}/tst";
require 'vstr_tst_examples.pl';

run_tst($_) for @tsts;

my $opts = "--mmap";
run_tst($_) for @tsts;

$opts = "--none";
run_tst($_, "ex_hexdump_none") for @tsts;

success();
