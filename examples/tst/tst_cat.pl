#! /usr/bin/perl -w

my @tsts = ("ex_cat");

use strict;

push @INC, "$ENV{SRCDIR}/tst";
require 'vstr_tst_examples.pl';

run_tst($_) for @tsts;

my $opts = "--mmap";
run_tst($_) for @tsts;

success();
