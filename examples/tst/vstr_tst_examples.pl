
use strict;

use File::Basename;
use File::Compare;

my $tst_DBG = 0;

my $tst_mp     = 1;
my $tst_num_mp = 2;

my $xit_success = 0;
my $xit_failure = 1;
my $xit_fail_ok = 77;

sub failure
  {
    my $txt = shift;

    warn("FAILURE($$) $0: $txt\n");
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

    print "TST: ${prefix}\n";

    print "DBG: files=@files\n" if ($tst_DBG);
    @files = undef;

    if (!$sz)
      { failure("NO files: ${prefix}"); }

    my @pids = ();
    for my $num (1..($sz * $tst_num_mp))
      {
	--$num; $num %= $sz; ++$num;
	if ($tst_mp)
	  {
	    my $pid = fork();
	
	    if (!defined ($pid))
	      { failure("fork: $!"); }

	    push @pids, $pid;
	    next    if ($pid);
	  }

	if (! -f "$dir/${prefix}_tst_$num")
	  { failure("NO file: $dir/${prefix}_tst_$num"); }
	if (! -f "$dir/${prefix}_out_$num")
	  { failure("NO file $dir/${prefix}_out_$num"); }

	my $fsz = -s "$dir/${prefix}_out_$num";

	unlink("${prefix}_tmp_${num}_$$");
	print "DBG: $dir/${prefix}_tst_$num ${prefix}_tmp_${num}\n" if ($tst_DBG);
	$func->("$dir/${prefix}_tst_$num", "${prefix}_tmp_${num}_$$",
		$xtra, $fsz);

	if (compare("$dir/${prefix}_out_$num", "${prefix}_tmp_${num}_$$") != 0)
	  {
	    rename("${prefix}_tmp_${num}_$$", "${prefix}_tmp_${num}");
	    failure("tst ${prefix} $num");
	  }
	unlink("${prefix}_tmp_${num}_$$");

	if ($tst_mp)
	  { _exit(0); }
      }

    for my $pid (@pids)
      {
	if (waitpid($pid, 0) <= 0)
	  { failure("wait($pid)"); }
	if ($? != $xit_success)
	  { failure("wait($pid)"); }
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

{
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
	/^PID: (\d+) ADDR (\d+[.]\d+[.]\d+[.]\d+)@(\d+)$/ || next;
	($daemon_pid, $daemon_addr, $daemon_port) = ($1, $2, $3);

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
    my $no_out = ">/dev/null";
    if ($tst_DBG)
      {
	$dbg    = '-d -d -d';
	$no_out = '';
      }

    system("./${cmd} $port $opts $cntl $dbg -- $args $no_out &");

    # Wait for it...
    while (! -e "${cmd}_cntl")
      { sleep(1); }

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

use POSIX;
use IO::Socket;
#require "sys/socket.ph";

sub nonblock {
    my $socket = shift;
    my $flags;

    $flags = fcntl($socket, F_GETFL, 0)
      or failure("Can't get flags for socket: $!");
    fcntl($socket, F_SETFL, $flags | O_NONBLOCK)
      or failure("Can't make socket nonblocking: $!");
}
sub daemon_connect_tcp
  {
    my $sock = new IO::Socket::INET->new(PeerAddr => daemon_addr(),
					 PeerPort => daemon_port(),
					 Proto    => "tcp",
					 Type     => SOCK_STREAM,
					 Timeout  => 2) ||
					   failure("connect: $!");
    nonblock($sock);
#    setsockopt($sock, SOL_SOCKET, TCP_NODELAY, 1) ||
#      failure("TCP_NODELAY: $!");
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
    my $sock = shift;
    my $data = shift;
    my $done_shutdown = shift;
    my $slow_write = shift;
    my $allow_write_truncate = shift;
    my $len = length($data);
    my $off = 0;
    my $output = '';

    # don't do shutdown by default....
    if (!defined($done_shutdown)) { $done_shutdown = 1; }

# This busy spins ... fuckit...
    while (1)
      {
	if ($len)
	  {
	    my $tmp = $len;

	    if ($slow_write && ($tmp > 1))
	      { $tmp /= 2; }
	    $tmp = $sock->syswrite($data, $tmp, $off);
	    if (!defined($tmp) && $allow_write_truncate &&
		($! eq 'Connection reset by peer'))
	      { $tmp = $len; }
	    if (!defined($tmp) && ($! ne 'Resource temporarily unavailable'))
	      { failure("write: $!"); }
	    if (defined($tmp))
	      {
		$len -= $tmp; $off += $tmp;
	      }
	  }

	my $ret = $sock->sysread(my($buff), 4096);

	next if (!defined($ret) &&
		 ($! eq 'Resource temporarily unavailable'));
	failure ("read: $!") if (!defined($ret));
	last if (!$ret);

	if (!$len && !$done_shutdown)
	  {
#	    if ($slow_write) # Makes this _slow_
#	      { select(undef, undef, undef, 0.50); }
	    $sock->shutdown(1);
	    $done_shutdown = 1;
	  }

	$output .= $buff;
      }

    return $output;
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
