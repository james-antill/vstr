
use strict;

use File::Basename;
use File::Compare;

my $xit_success = 0;
my $xit_failure = 1;
my $xit_fail_ok = 77;

sub failure
  {
    my $txt = shift;

    warn("$0: $txt\n");
    exit($xit_failure);
  }

sub success
  {
    exit($xit_success);
  }

my $dir = $ENV{SRCDIR};

sub run_tst
  {
    my $cmd    = shift;
    my $prefix = shift;
    my @files  = <$dir/${cmd}_tst_*>;
    my $sz     = scalar @files;

    @files = undef;

    if (!defined($prefix)) { $prefix = $cmd; }

    for my $num (1..$sz)
      {
	if (! -f "$dir/${prefix}_tst_$num" ||
	    ! -f "$dir/${prefix}_out_$num")
	  { failure("files $prefix $num"); }

	my $opts = $main::opts;

	system("${cmd} $opts $dir/${prefix}_tst_$num > ${prefix}_tmp_$num");

	if (compare("$dir/${prefix}_out_$num", "${prefix}_tmp_$num") != 0)
	  { failure("tst $prefix $num"); }

	unlink("${prefix}_tmp_$num");
      }
  }

1;
