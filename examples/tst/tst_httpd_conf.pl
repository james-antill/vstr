#! /usr/bin/perl -w

use strict;

push @INC, "$ENV{SRCDIR}/tst";

require 'httpd_tst_utils.pl';

our $truncate_segv;
our $root;

setup();

$truncate_segv = $ENV{VSTR_TST_HTTP_TRUNC_SEGV};
$truncate_segv = 1 if (!defined ($truncate_segv));

# quick tests...
conf_tsts(5, 5);
success();

my $old_truncate_segv = $truncate_segv;
$truncate_segv = 1; # Stop gen tests to save time...
conf_tsts(1, 1);

conf_tsts(2, 2);
# conf_tsts(1, 2);

conf_tsts(3, 3);
# conf_tsts(1, 3);

conf_tsts(4, 4);

conf_tsts(5, 5);

$truncate_segv = $old_truncate_segv;
conf_tsts(1, 5);

rmtree($root);

success();

