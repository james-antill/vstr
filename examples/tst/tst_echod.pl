#! /usr/bin/perl -w

use strict;
use File::Path;

push @INC, "$ENV{SRCDIR}/tst";
require 'vstr_tst_examples.pl';

sub sub_http_tst
  {
    my $io_r = shift;
    my $io_w = shift;
    my $xtra = shift || {};
    my $sz   = shift;

    my $sock = daemon_connect_tcp();
    my $data = daemon_get_io_r($io_r);

    my $output = daemon_io($sock, $data,
			   $xtra->{shutdown_w}, $xtra->{slow_write});
    daemon_put_io_w($io_w, $output);
  }

sub all_tsts()
  {
    sub_tst(\&sub_http_tst, "ex_echod", {shutdown_w => 0});
    sub_tst(\&sub_http_tst, "ex_echod", {shutdown_w => 0, slow_write => 1});
  }

if (@ARGV)
  {
    daemon_status(shift);
    all_tsts();
    success();
  }

run_tst("ex_echod", "ex_echod_help", "--help");
run_tst("ex_echod", "ex_echod_version", "--version");

daemon_init("ex_echod");
all_tsts();
daemon_exit("ex_echod");

success();

END {
  my $save_exit_code = $?;
  daemon_cleanup("ex_echod");
  $? = $save_exit_code;
}
