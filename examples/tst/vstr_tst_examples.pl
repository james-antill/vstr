
use strict;

use File::Basename;
use File::Compare;

my $tst_DBG = 0;

my $xit_success = 0;
my $xit_failure = 1;
my $xit_fail_ok = 77;

sub failure
  {
    my $txt = shift;

    warn("FAILURE $0: $txt\n");
    exit($xit_failure);
  }

sub success
  {
    exit($xit_success);
  }

my $dir = "$ENV{SRCDIR}/tst";

sub sub_tst
  {
    my $func   = shift;
    my $prefix = shift;
    my $xtra   = shift;
    my @files  = <$dir/${prefix}_tst_*>;
    my $sz     = scalar @files;

    print "DBG: files=@files\n" if ($tst_DBG);
    @files = undef;

    for my $num (1..$sz)
      {
	if (! -f "$dir/${prefix}_tst_$num" ||
	    ! -f "$dir/${prefix}_out_$num")
	  { failure("files $dir/${prefix}_tst_$num $dir/${prefix}_out_$num"); }

	my $fsz = -s "$dir/${prefix}_out_$num";

	unlink("${prefix}_tmp_$num");
	print "DBG: $dir/${prefix}_tst_$num ${prefix}_tmp_$num\n" if ($tst_DBG);
	$func->("$dir/${prefix}_tst_$num", "${prefix}_tmp_$num", $xtra, $fsz);

	if (compare("$dir/${prefix}_out_$num", "${prefix}_tmp_$num") != 0)
	  { failure("tst $dir/${prefix}_tst_$num ${prefix}_tmp_$num"); }
	unlink("${prefix}_tmp_$num");
      }
  }

sub run_tst
  {
    my $cmd    = shift;
    my $prefix = shift;

    if (!defined($prefix)) { $prefix = $cmd; }

    sub sub_run_tst
      {
	my $io_r = shift;
	my $io_w = shift;
	my $xtra = shift;

	my $cmd  = $xtra->{cmd};
	my $opts = $xtra->{opts};

	system("./${cmd} $opts -- $io_r > $io_w");
      }
    sub sub_run_pipe_tst
      {
	my $io_r = shift;
	my $io_w = shift;
	my $xtra = shift;

	my $cmd  = $xtra->{cmd};
	my $opts = $xtra->{opts};

	system("./ex_cat $io_r | ./${cmd} $opts > $io_w");
      }

    my $opts = $main::opts || "";
    sub_tst(\&sub_run_tst,      $prefix, {cmd => $cmd, opts => $opts});
    sub_tst(\&sub_run_pipe_tst, $prefix, {cmd => $cmd, opts => $opts});
  }

{
my $daemon_addr = undef;
my $daemon_port = undef;
sub daemon_init
  {
    my $cmd    = shift;

    my $args   = shift || '';
    my $opts   = shift || "";

    my $cntl = "--cntl-file=${cmd}_cntl";
    my $port = "--port=0"; # Rand

    system("./${cmd} $port $opts $cntl -- $args >/dev/null 2>/dev/null &");

    open(INFO, "./ex_cntl -e status ${cmd}_cntl |") ||
      failure("Can't open control for ${cmd}.");

    while (<INFO>)
      {
	/^PID: \d+ ADDR (\d+[.]\d+[.]\d+[.]\d+)@(\d+)$/ || next;
	($daemon_addr, $daemon_port) = ($1, $2);

	if ($daemon_addr eq '0.0.0.0')
	  {
	    $daemon_addr = 'localhost';
	  }
	last;
      }

    close(INFO);
  }

sub daemon_addr
  {
    return $daemon_addr;
  }
sub daemon_port
  {
    return $daemon_port;
  }

sub daemon_exit
  {
    my $cmd    = shift;

    system("./ex_cntl -e close ${cmd}_cntl > /dev/null");
    unlink("${cmd}_cntl");
    $daemon_addr = undef;
    $daemon_port = undef;
  }
sub daemon_cleanup
  {
    my $cmd    = shift;

    if (-e "${cmd}_cntl")
      {
	daemon_exit($cmd)
      }
  }
}

1;
