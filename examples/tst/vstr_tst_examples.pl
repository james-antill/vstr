
use strict;

use File::Basename;
use File::Compare;

use POSIX;

use IO::Socket;
use IO::Handle;

my $tst_DBG    = $ENV{VSTR_TST_DBG};
   $tst_DBG    = 0 if (!defined ($tst_DBG));

my $tst_mp     = $ENV{VSTR_TST_MP};
   $tst_mp     = 1 if (!defined ($tst_mp));
my $tst_num_mp = $ENV{VSTR_TST_NUM_MP};
   $tst_num_mp = 2 if (!defined ($tst_num_mp));

my $child_no_cleanup = 0;

my $xit_success = 0;
my $xit_failure = 1;
my $xit_fail_ok = 77;

STDOUT->autoflush(1);
STDERR->autoflush(1);

sub failure
  {
    my $txt = shift;

    warn("FAILURE($$) $0: $txt\n");
    if ($child_no_cleanup)
      { _exit($xit_failure); }
    else
      { exit($xit_failure); }
  }

sub success
  {
    if ($child_no_cleanup)
      { _exit($xit_success); }
    else
      { exit($xit_success); }
  }

sub tst_fork()
  {
    my $pid = fork();
    if (defined($pid) && !$pid)
      { $child_no_cleanup = 1; }

    return $pid;
  }

my $tst__proc_limit = 150;

sub tst_proc_limit
  {
    $tst__proc_limit = shift;
  }
my @TST__PROCS = ();
sub tst_proc_wait()
  {
    print "DBG($$): BEG wait() = [" . @TST__PROCS . "]\n" if ($tst_DBG > 2);

    my $pid = shift @TST__PROCS;

    if (waitpid($pid, 0) <= 0)
      { failure("waitpid($pid): $!"); }
    my $code = $?;
    # 13 seems to be some weird perl error code.
    if (($code != $xit_success) && ($code != 13))
      { failure("waitpid($pid) == $code"); }
    print "DBG($$): END wait($pid) = [" . @TST__PROCS . "]\n" if ($tst_DBG > 2);
  }

sub tst_proc_fork()
  {
    if (@TST__PROCS >= $tst__proc_limit)
      {
	tst_proc_wait();
      }

    print "DBG($$): BEG fork() = [" . @TST__PROCS . "]\n" if ($tst_DBG > 2);
    my $pid = tst_fork();

    if (!defined ($pid))
      { failure("fork: $!"); }

    if ($pid)
      {
	push @TST__PROCS, $pid;
	print "DBG($$): END fork($pid) = [" . @TST__PROCS . "]\n" if ($tst_DBG > 2);
      }
    return $pid;
  }

my $dir = "$ENV{SRCDIR}/tst";

sub sub_tst
  {
    my $func   = shift;
    my $prefix = shift;
    my $xtra   = shift;
    my @files  = <$dir/${prefix}_tst_*>;
    my $sz     = scalar @files;

    print "TST: ${prefix}\n";

    print "DBG($$): files=@files\n" if ($tst_DBG);
    @files = undef;

    if (!$sz)
      { failure("NO files: ${prefix}"); }

    for my $num (1..($sz * $tst_num_mp))
      {
	--$num; $num %= $sz; ++$num;
	if ($tst_mp)
	  {
	    my $pid = tst_proc_fork();
	    next    if ($pid);
	  }

	if (! -f "$dir/${prefix}_tst_$num")
	  { failure("NO file: $dir/${prefix}_tst_$num"); }
	if (! -f "$dir/${prefix}_out_$num")
	  { failure("NO file $dir/${prefix}_out_$num"); }

	my $fsz = -s "$dir/${prefix}_out_$num";

	unlink("${prefix}_tmp_${num}_$$");
	print "DBG($$): $dir/${prefix}_tst_$num ${prefix}_tmp_${num}\n" if ($tst_DBG);
	$func->("$dir/${prefix}_tst_$num", "${prefix}_tmp_${num}_$$",
		$xtra, $fsz);

	if (compare("$dir/${prefix}_out_$num", "${prefix}_tmp_${num}_$$") != 0)
	  {
	    rename("${prefix}_tmp_${num}_$$", "${prefix}_tmp_${num}");
	    failure("tst ${prefix} $num");
	  }
	unlink("${prefix}_tmp_${num}_$$");

	if ($tst_mp)
	  { success(); }
      }

    while (@TST__PROCS)
      {
	tst_proc_wait();
      }
  }

sub run_tst
  {
    my $cmd    = shift;
    my $prefix = shift || $cmd;
    my $opts   = shift || "";

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

	system("./ex_slowcat -b 64 -s 0 -u 40   $io_r | " .
	       "./${cmd} $opts | " .
	       "./ex_slowcat -b 32 -s 0 -u 40 > $io_w");
      }

    sub_tst(\&sub_run_tst,      $prefix, {cmd => $cmd, opts => $opts});
    sub_tst(\&sub_run_pipe_tst, $prefix, {cmd => $cmd, opts => $opts});
  }

sub run_simple_tst
  {
    my $cmd    = shift;
    my $prefix = shift || $cmd;

    sub sub_run_simple_tst
      {
	my $io_r = shift;
	my $io_w = shift;
	my $xtra = shift;

	my $cmd  = $xtra->{cmd};

	system("./${cmd} $io_r > $io_w");
      }

    sub_tst(\&sub_run_simple_tst, $prefix, {cmd => $cmd});
  }

{
my $ldaemon_pid = undef;
my $daemon_pid  = undef;
my $daemon_addr = undef;
my $daemon_port = undef;
sub daemon_status
  {
    my $cntl = shift;

    open(INFO, "./ex_cntl -e status ${cntl} |") ||
      failure("Can't open control ${cntl}.");

    while (<INFO>)
      {
	/^STATUS: / || next;
	/from\[(\d+[.]\d+[.]\d+[.]\d+)@(\d+)\]/ || next;
	($daemon_addr, $daemon_port) = ($1, $2);
	/pid\[(\d+)\]$/ || next;
	$daemon_pid = $1;

	if ($daemon_addr eq '0.0.0.0')
	  {
	    $daemon_addr = '127.0.0.1';
	  }
	last;
      }

    close(INFO) || failure("Problem with cntl ${cntl}.");
  }

sub daemon_init
  {
    my $cmd    = shift;

    my $args   = shift || '';
    my $opts   = shift || "";

    my $cntl = "--cntl-file=${cmd}_cntl";
    my $port = "--port=0"; # Rand

    my $dbg = "";
    my $no_out = ">/dev/null  2> /dev/null";
    if ($tst_DBG > 1)
      {
	$no_out = '';
	if ($tst_DBG > 2)
	  { $dbg    = '-d -d -d'; }
      }

    unlink("${cmd}_cntl"); # So we don't try connecting to the old one
    print "TST: ${cmd} $opts -- $args\n";

    $ldaemon_pid = tst_fork();
    if (!defined ($ldaemon_pid))
      { failure("fork($daemon_pid): $!"); }

    if (!$ldaemon_pid)
      {
	if (system("./${cmd} $port $opts $cntl $dbg -- $args $no_out"))
	  { failure("daemon($cmd): $!"); }
	success("daemon($cmd)");
      }

    # Wait for it...
    my $num = 0;
    while (! -e "${cmd}_cntl")
      {
	if (++$num > (60 * 5))
	  { failure("Can't find ${cmd}_cntl file"); }
	sleep(1);
      }

    daemon_status("${cmd}_cntl");
  }

sub daemon_pid
  {
    return $daemon_pid;
  }
sub daemon_addr
  {
    return $daemon_addr;
  }
sub daemon_port
  {
    return $daemon_port;
  }

sub nonblock {
    my $socket = shift;
    my $val = shift;
    my $flags;

    if (!defined($val))
      { $val = 1; }

    $flags = fcntl($socket, F_GETFL, 0)
      or failure("Can't get flags for socket: $!");

    return if (!!($flags & O_NONBLOCK) == !!$val);

    if ($val)
      { $flags |= O_NONBLOCK; }
    else
      { $flags &= ~O_NONBLOCK; }

    fcntl($socket, F_SETFL, $flags)
      or failure("Can't make socket nonblocking: $!");
}
sub daemon_connect_tcp
  {
    my $sock = new IO::Socket::INET->new(PeerAddr => daemon_addr(),
					 PeerPort => daemon_port(),
					 Proto    => "tcp",
					 Type     => SOCK_STREAM,
					 Timeout  => (2 * $tst_num_mp)) ||
					   failure("connect: $!");
    nonblock($sock);
    print "DBG($$): created sock " . $sock->fileno . "\n" if ($tst_DBG > 2);
    return $sock;
  }

sub daemon_get_io_r
  {
    my $io_r = shift;

    my $data_r = "";
    { local ($/);
      open(IO_R, "< $io_r") || failure("open $io_r: $!");
      $data_r = <IO_R>;
      close(IO_R);
    }

    return $data_r;
  }

sub daemon_put_io_w
  {
    my $io_w   = shift;
    my $output = shift;

    open(IO_W, "> $io_w") || failure("open $io_w: $!");
    print IO_W $output;
    close(IO_W) || failure("close $io_w: $!");
  }

sub daemon_io
  {
    my $data = shift;
    my $done_shutdown = shift;
    my $slow_write = shift;
    my $allow_write_truncate = shift;
    my $len = length($data);
    my $sock = daemon_connect_tcp();
    my $off = 0;
    my $output = '';

    # don't do shutdown by default....
    if (!defined($done_shutdown)) { $done_shutdown = 1; }

    my $fin_write = !$len;
    while (1)
      {
	if ($len)
	  {
	    # Block...
	    {
	      my $rin = '';
	      my $win = '';
	      vec($rin,fileno($sock),1) = 1;
	      vec($win,fileno($sock),1) = 1;
	      my $ein = $rin | $win;
	      select($rin, $win, $ein, undef);
	    }

	    my $wret = $len;

	    if ($slow_write && ($wret > 1))
	      { $wret /= 2; }
	    $wret = $sock->syswrite($data, $wret, $off);
	    if (!defined($wret) && $allow_write_truncate &&
		($! eq 'Connection reset by peer'))
	      { $wret = $len; }
	    if (!defined($wret) && ($! ne 'Resource temporarily unavailable'))
	      { failure("write: $!"); }
	    if (defined($wret))
	      {
		$len -= $wret; $off += $wret;
		if (!$len)
		  { $fin_write = 1; }
	      }
	  }
	if ($fin_write)
	  {
	    nonblock($sock, 0);
	    if (!$done_shutdown)
	      {
		$sock->shutdown(1);
		$done_shutdown = 1;
	      }
	    $fin_write = 0;
	  }

	my $ret = $sock->sysread(my($buff), 4096);

	next if (!defined($ret) &&
		 ($! eq 'Resource temporarily unavailable'));
	last if (!defined($ret) &&
		 ($! eq 'Connection reset by peer'));
	failure ("read: $!") if (!defined($ret));
	last if (!$ret);

	$output .= $buff;
      }

    $sock->close();

    return $output;
}

sub daemon_exit
  {
    my $cmd    = shift;

    if (system("./ex_cntl -e close ${cmd}_cntl > /dev/null"))
      { warn "cntl close: $!\n"; }
    unlink("${cmd}_cntl");
    $daemon_addr = undef;
    $daemon_port = undef;
    if (defined ($ldaemon_pid))
      {
	if (waitpid($ldaemon_pid, 0) <= 0)
	  { warn "waitpid(ldaemon[$ldaemon_pid]): $!\n"; }

	my $code = $?;
	if ($code != $xit_success)
	  { warn "waitpid(ldaemon[$ldaemon_pid]) == $code\n"; }
      }
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
