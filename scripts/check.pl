#! /usr/bin/perl -w

use strict;
use FileHandle;

# Do a fast check of the main options ?
my $conf_fast_check = 1;
# Do a full check, or group all disable options together
my $conf_full_check = 0;

# From above.
# 1, 0
# CC=gcc3 $0  3395.39s user 1056.41s system 82% cpu 1:29:52.09 total
#
# 1, 1
#         $0  22581.08s user 7515.35s system 91% cpu 9:10:16.59 total
# CC=gcc3 $0  24451.63s user 7777.30s system 91% cpu 9:49:38.91 total


my @C_ls =     ("--enable-linker-script");
my @C_dbg =    ("--enable-debug");
my @C_np =     ("--enable-noposix-host");
my @C_nin =    ("--enable-tst-noinline");
my @C_natals = ("--enable-tst-noattr-alias");
my @C_natvis = ("--enable-tst-noattr-visibility");
my @C_dbl_g =  ("--with-fmt-float=glibc");
my @C_dbl_h =  ("--with-fmt-float=host");
my @C_dbl_n =  ("--with-fmt-float=none");

$SIG{INT} = sub { exit (1); };

my %remap = ();

sub print_cc_cflags()
  {
    my $cc = "gcc";       if (defined ($ENV{CC}))     { $cc = $ENV{CC}; }
    my $cf = "<default>"; if (defined ($ENV{CFLAGS})) { $cf = $ENV{CFLAGS}; }

    FOUT->print("CC=\"$cc\"\n");
    FOUT->print("CFLAGS=\"$cf\"\n");
    FERR->print("CC=\"$cc\"\n");
    FERR->print("CFLAGS=\"$cf\"\n");
  }

sub conf
{
  my $p_args = join " ", @_;

  $p_args =~ s/--enable-//g;
  $p_args =~ s/--with-//g;

  FOUT->print("\n");
  FOUT->print("\n");
  FOUT->print("==== BEG: $p_args ====\n");
  FOUT->print("\n");

  FERR->print("\n");
  FERR->print("==== BEG: $p_args ====\n");

  if (!open(OLD_STDOUT, ">&STDOUT"))
    { die "dup2(OLD, STDOUT): $!\n"; }
  if (!open(OLD_STDERR, ">&STDERR"))
    { die "dup2(OLD, STDERR): $!\n"; }

  if (!open(STDOUT, ">&FOUT"))
    { die "dup2(STDOUT, FOUT): $!\n"; }
  if (!open(STDERR, ">&FERR"))
    { die "dup2(STDERR, FERR): $!\n"; }

  my $ok = 0;
  if (!system("./configure", @_) &&
      !system("make", "clean") &&
      !system("make", "check"))
    {
      # Fear the power of sh...
      if (!open(STDOUT, ">&FERR"))
	{ die "dup2(STDOUT, FERR): $!\n"; }

      if (system("./scripts/valgrind.sh | egrep -C 2 '^=='"))
	{
	  $ok = 1;
	}
    }

  if (!$ok)
    {
      FOUT->print("*" x 35 . " BAD " . "*" x 35 . "\n");
      FERR->print("*" x 35 . " BAD " . "*" x 35 . "\n");
      sleep(4);
    }

  FOUT->print("==== END: $p_args ====\n");
  FERR->print("==== END: $p_args ====\n");

  if (!open(STDOUT, ">&OLD_STDOUT"))
    { die "dup2(FOUT): $!\n"; }
  if (!open(STDERR, ">&OLD_STDERR"))
    { die "dup2(FERR): $!\n"; }

  close(OLD_STDOUT);
  close(OLD_STDERR);
}

sub t_U
{ # This accounts for the remap hash...
  my @a = @_;

  @a = sort @a;

  for (@a)
    {
      if (defined ($remap{$_}))
	{ $_ = $remap{$_}; }
    }

  my $last = undef;
  for my $i (@a)
    {
      if (defined ($last) && ($last == $i))
	{ return (0); }
      $last = $i;
    }

  return (1);
}

sub tst_conf_1
{ # Pick all combs. of 1 each
  for my $i (0..$#_)
    {
      conf (@{$_[$i]});
    }
}

sub tst_conf_2
{ # Pick all combs. of 2 each
    our @a = ();
    for my $i ( 0..$#_) { local @a = (@a, $i);
    for my $j ($i..$#_) { local @a = (@a, $j); if (t_U @a) {
      conf (@{$_[$i]}, @{$_[$j]});
    } } }
}

sub tst_conf_3
  {
    our @a = ();
    for my $i ( 0..$#_) { local @a = (@a, $i);
    for my $j ($i..$#_) { local @a = (@a, $j); if (t_U @a) {
    for my $k ($j..$#_) { local @a = (@a, $k); if (t_U @a) {
      conf (@{$_[$i]}, @{$_[$j]}, @{$_[$k]});
    } } } } }
  }

sub tst_conf_4
  {
    our @a = ();
    for my $i ( 0..$#_) { local @a = (@a, $i);
    for my $j ($i..$#_) { local @a = (@a, $j); if (t_U @a) {
    for my $k ($j..$#_) { local @a = (@a, $k); if (t_U @a) {
    for my $l ($k..$#_) { local @a = (@a, $l); if (t_U @a) {
      conf (@{$_[$i]}, @{$_[$j]}, @{$_[$k]},
	    @{$_[$l]});
    } } } } } } }
  }

sub tst_conf_5
  {
    our @a = ();
    for my $i ( 0..$#_) { local @a = (@a, $i);
    for my $j ($i..$#_) { local @a = (@a, $j); if (t_U @a) {
    for my $k ($j..$#_) { local @a = (@a, $k); if (t_U @a) {
    for my $l ($k..$#_) { local @a = (@a, $l); if (t_U @a) {
    for my $m ($l..$#_) { local @a = (@a, $m); if (t_U @a) {
      conf (@{$_[$i]}, @{$_[$j]}, @{$_[$k]},
	    @{$_[$l]}, @{$_[$m]});
    } } } } } } } } }
  }

sub tst_conf_6
  {
    our @a = ();
    for my $i ( 0..$#_) { local @a = (@a, $i);
    for my $j ($i..$#_) { local @a = (@a, $j); if (t_U @a) {
    for my $k ($j..$#_) { local @a = (@a, $k); if (t_U @a) {
    for my $l ($k..$#_) { local @a = (@a, $l); if (t_U @a) {
    for my $m ($l..$#_) { local @a = (@a, $m); if (t_U @a) {
    for my $n ($m..$#_) { local @a = (@a, $n); if (t_U @a) {
      conf (@{$_[$i]}, @{$_[$j]}, @{$_[$k]},
	    @{$_[$l]}, @{$_[$m]}, @{$_[$n]});
    } } } } } } } } } } }
  }

sub tst_conf_7
  { # Pick all combs. of 7 each
    our @a = ();
    for my $i ( 0..$#_) { local @a = (@a, $i);
    for my $j ($i..$#_) { local @a = (@a, $j); if (t_U @a) {
    for my $k ($j..$#_) { local @a = (@a, $k); if (t_U @a) {
    for my $l ($k..$#_) { local @a = (@a, $l); if (t_U @a) {
    for my $m ($l..$#_) { local @a = (@a, $m); if (t_U @a) {
    for my $n ($m..$#_) { local @a = (@a, $n); if (t_U @a) {
    for my $o ($n..$#_) { local @a = (@a, $o); if (t_U @a) {
      conf (@{$_[$i]}, @{$_[$j]}, @{$_[$k]},
	    @{$_[$l]}, @{$_[$m]}, @{$_[$n]}, @{$_[$o]});
    } } } } } } } } } } } } }
  }

# -------------------------------------------------------------
# -------------------------------------------------------------

if (scalar(@ARGV) && ($ARGV[0] eq "yes"))
  {
    if (!open(FOUT, ">> autocheck1.log"))
      { die "open(autocheck1.log): $!\n"; }

    if (!open(FERR, ">> autocheck2.log"))
      { die "open(autocheck2.log): $!\n"; }
  }
else
  {
    if (!open(FOUT, "> autocheck1.log"))
      { die "open(autocheck1.log): $!\n"; }

    if (!open(FERR, "> autocheck2.log"))
      { die "open(autocheck2.log): $!\n"; }
  }

print_cc_cflags();

# Do normal, then nothing, hopefully spots errors quicker
#  also checked later on though.
if ($conf_fast_check)
  {
    conf(@C_ls);
    conf(@C_ls, @C_dbg);
  }

if ($conf_full_check)
  {
    # Do all options...

    $remap{7} = 6;
    $remap{8} = 6;
    tst_conf_7 (\@C_ls, \@C_dbg, \@C_nin, \@C_natals, \@C_natvis, \@C_np,
		\@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_6 (\@C_ls, \@C_dbg, \@C_nin, \@C_natals, \@C_natvis, \@C_np,
		\@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_5 (\@C_ls, \@C_dbg, \@C_nin, \@C_natals, \@C_natvis, \@C_np,
		\@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_4 (\@C_ls, \@C_dbg, \@C_nin, \@C_natals, \@C_natvis, \@C_np,
		\@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_3 (\@C_ls, \@C_dbg, \@C_nin, \@C_natals, \@C_natvis, \@C_np,
		\@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_2 (\@C_ls, \@C_dbg, \@C_nin, \@C_natals, \@C_natvis, \@C_np,
		\@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_1 (\@C_ls, \@C_dbg, \@C_nin, \@C_natals, \@C_natvis, \@C_np,
		\@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
  }
else
  {
    # Group all "turn off" flags as one...

    my @tmp_no = (@C_nin, @C_natals, @C_natvis, @C_np);

    $remap{4} = 3;
    $remap{5} = 3;
    tst_conf_4 (\@C_ls, \@C_dbg, \@tmp_no, \@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_3 (\@C_ls, \@C_dbg, \@tmp_no, \@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_2 (\@C_ls, \@C_dbg, \@tmp_no, \@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
    tst_conf_1 (\@C_ls, \@C_dbg, \@tmp_no, \@C_dbl_g, \@C_dbl_h, \@C_dbl_n);
  }


# Make sure that the documentation is up to date.
chdir("Documentation");
system("./txt2html.pl");
system("./txt2man.pl");

exit (0);
