#! /usr/bin/perl -w

use strict;
use File::Path;
use File::Copy;

push @INC, "$ENV{SRCDIR}/tst";
require 'vstr_tst_examples.pl';

our $root = "ex_httpd_root";
our $truncate_segv = 0;

sub http_cntl_list_beg
  { # FIXME: see if it looks "OK"
    my $list_pid = tst_fork();
    if (defined ($list_pid) && !$list_pid) {
      sleep(2);
      system("./ex_cntl -e list ex_httpd_cntl > /dev/null");
      _exit(0);
    }
    return $list_pid;
  }
sub http_cntl_list_cleanup
  {
    my $list_pid = shift;
    if (defined($list_pid))
      { waitpid($list_pid, 0); }
  }

sub httpd__munge_ret
  {
    my $output = shift;

    # Remove date, because that changes each time
    $output =~ s/^(Date:).*$/$1/gm;
    # Remove last-modified = start date for error messages
    $output =~
      s!(HTTP/1[.]1 \s (?:30[1237]|40[03456]|41[0234567]|50[0135]) .*)$ (\n)
	^(Date:)$ (\n)
	^(Server:.*)$ (\n)
	^(Last-Modified:) .*$
	!$1$2$3$4$5$6$7!gmx;
    # Remove last modified for trace ops
    $output =~
      s!^(Last-Modified:).*$ (\n)
        ^(Content-Type: \s message/http.*)$
	!$1$2$3!gmx;

    return $output;
  }

sub httpd_file_tst
  {
    my $io_r = shift;
    my $io_w = shift;
    my $xtra = shift || {};
    my $sz   = shift;

    my $sock = daemon_connect_tcp();
    my $data = daemon_get_io_r($io_r);

    $data =~ s/\n/\r\n/g;

    my $output = daemon_io($sock, $data,
			   $xtra->{shutdown_w}, $xtra->{slow_write}, 1);

    $output = httpd__munge_ret($output);
    daemon_put_io_w($io_w, $output);
  }

sub httpd_gen_tst
  {
    my $io_r = shift;
    my $io_w = shift;
    my $xtra = shift || {};
    my $sz   = shift;

    my $sock = daemon_connect_tcp();
    my $data = daemon_get_io_r($io_r);

    if (length($data) != 0)
      { failure(sprintf("data(%d) on gen tst", length($data))); }

    if (! exists($xtra->{gen_output}))
      { $xtra->{gen_output} = \&httpd__munge_ret; }

    $data = $xtra->{gen_input}->();

    my $output = daemon_io($sock, $data,
			   $xtra->{shutdown_w}, $xtra->{slow_write}, 1);

    $output = $xtra->{gen_output}->($output);

    daemon_put_io_w($io_w, $output);
  }

sub gen_tst_e2big
  {
    my $gen_cb = sub {
      my $data = ("\r\n" x 80_000) . ("x" x 150_000);
      return $data;
    };

    my $gen_out_cb = sub { # Load ex_httpd_null_out_1 ?
      $_ = shift;
      if (m!^HTTP/1.1 400 !)
	{
	  $_ = "";
	}

      return $_;
    };

    sub_tst(\&httpd_gen_tst, "ex_httpd_null",
	    {gen_input => $gen_cb, gen_output => $gen_out_cb,
	     shutdown_w => 0});
  }

use POSIX; # _exit

sub gen_tst_trunc
  {
    return if ($main::truncate_segv);

    my $vhosts = shift;
    my $pid = 0;

    if (!($pid = tst_fork()) && defined($pid))
      {
	if (1)
	  {
	    open(STDIN,  "< /dev/null") || failure("open(2): $!");
	    open(STDOUT, "> /dev/null") || failure("open(2): $!");
	    open(STDERR, "> /dev/null") || failure("open(2): $!");
	  }

	my $fname = "$main::root/foo.example.com/4mb_2_2mb";

	if (!$vhosts)
	  {
	    $fname = "$main::root/4mb_2_2mb";
	  }

	if (($pid = fork()) || !defined($pid))
	  { # Parent goes
	    sleep(4);
	    truncate($fname, 2_000_000);
	    success();
	  }

	open(OUT, ">> $fname") || failure("open($fname): $!");

	truncate($fname, 4_000_000);

	my $gen_cb = sub {
	  sleep(1);
	  my $pad = "x" x 64_000;
	  my $data = <<EOL;
GET http://foo.example.com/4mb_2_2mb HTTP/1.1\r
Host: $pad\r
\r
EOL
	  $data = $data x 16;
	  return $data;
	};

	my $gen_out_cb = sub { # Load ex_httpd_null_out_1 ?
	  unlink($fname);
	  success();
	};
	# Randomly test as other stuff happens...
	sub_tst(\&httpd_gen_tst, "ex_httpd_null",
		{gen_input => $gen_cb, gen_output => $gen_out_cb,
		 shutdown_w => 0});
      }
  }

sub gen_tsts
  {
    my $vhosts = shift;

    gen_tst_trunc($vhosts);
    gen_tst_e2big();
  }

sub all_vhost_tsts()
  {
    gen_tsts(1);
    sub_tst(\&httpd_file_tst, "ex_httpd");
    if ($>) { # mode 000 doesn't work if running !uid
    sub_tst(\&httpd_file_tst, "ex_httpd_nonroot"); }

    sub_tst(\&httpd_file_tst, "ex_httpd_errs");

    sub_tst(\&httpd_file_tst, "ex_httpd",
	    {shutdown_w => 0});
    if ($>) {
    sub_tst(\&httpd_file_tst, "ex_httpd_nonroot",
	    {shutdown_w => 0}); }
    sub_tst(\&httpd_file_tst, "ex_httpd_errs",
	    {shutdown_w => 0});
    sub_tst(\&httpd_file_tst, "ex_httpd_shut",
	    {shutdown_w => 0});

    sub_tst(\&httpd_file_tst, "ex_httpd",
	    {                 slow_write => 1});
    if ($>) {
    sub_tst(\&httpd_file_tst, "ex_httpd_nonroot",
	    {                 slow_write => 1}); }
    sub_tst(\&httpd_file_tst, "ex_httpd_errs",
	    {                 slow_write => 1});

    sub_tst(\&httpd_file_tst, "ex_httpd",
	    {shutdown_w => 0, slow_write => 1});
    if ($>) {
    sub_tst(\&httpd_file_tst, "ex_httpd_nonroot",
	    {shutdown_w => 0, slow_write => 1}); }
    sub_tst(\&httpd_file_tst, "ex_httpd_errs",
	    {shutdown_w => 0, slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_shut",
	    {shutdown_w => 0, slow_write => 1});
  }

sub all_nonvhost_tsts()
  {
    gen_tsts(0);
    sub_tst(\&httpd_file_tst, "ex_httpd_non-virtual-hosts");
    sub_tst(\&httpd_file_tst, "ex_httpd_non-virtual-hosts",
	    {shutdown_w => 0});
    sub_tst(\&httpd_file_tst, "ex_httpd_non-virtual-hosts",
	    {                 slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_non-virtual-hosts",
	    {shutdown_w => 0, slow_write => 1});
  }

sub all_public_only_tsts
  {
    if (!@_) { gen_tsts(1); }
    sub_tst(\&httpd_file_tst, "ex_httpd_public-only");
    sub_tst(\&httpd_file_tst, "ex_httpd_public-only",
	    {shutdown_w => 0});
    sub_tst(\&httpd_file_tst, "ex_httpd_public-only",
	    {                 slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_public-only",
	    {shutdown_w => 0, slow_write => 1});
  }

sub all_none_tsts()
  {
    gen_tsts(1);
    sub_tst(\&httpd_file_tst, "ex_httpd_none");
    sub_tst(\&httpd_file_tst, "ex_httpd_none",
	    {shutdown_w => 0});
    sub_tst(\&httpd_file_tst, "ex_httpd_none",
	    {                 slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_none",
	    {shutdown_w => 0, slow_write => 1});
  }

if (@ARGV)
  {
    daemon_status(shift);
    if (@ARGV && (($ARGV[0] eq "virtual-hosts") || ($ARGV[0] eq "vhosts")))
      { all_vhost_tsts(); }
    elsif (@ARGV && ($ARGV[0] eq "public"))
      { all_public_only_tsts(); }
    elsif (@ARGV && ($ARGV[0] eq "none"))
      { all_none_tsts(); }
    else
      {	all_nonvhost_tsts(); }
    success();
  }

rmtree($root);
mkpath([$root . "/default",
	$root . "/default.example.com",
	$root . "/blah",
	$root . "/foo.example.com/nxt",
	$root . "/foo.example.com/corner/index.html",
	$root . "/foo.example.com:1234"]);

sub munge_mtime
  {
    my $num   = shift;
    my $fname = shift;

    my ($a, $b, $c, $d,
	$e, $f, $g, $h,
	$atime, $mtime) = stat("$ENV{SRCDIR}/tst/ex_httpd_tst_1");
    $atime -= ($num * (60 * 60 * 24));
    $mtime -= ($num * (60 * 60 * 24));
    utime $atime, $mtime, $fname;
  }

sub make_html
  {
    my $num   = shift;
    my $val   = shift;
    my $fname = shift;

    open(OUT, "> $fname") || failure("open $fname: $!");
    print OUT <<EOL;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<html>
  <head>
    <title>Foo $val</title>
  </head>
  <body>
    <h1>Foo $val</h1>
  </body>
</html>
EOL
    close(OUT) || failure("close");

    munge_mtime($num, $fname);
  }

my $big = "";

# Needs to be big or the .bz2 file won't stay around due to the 95% rule
$big .= ("\n" . ("x" x 10) . ("xy" x 10) . ("y" x 10)) x 500;
$big .= "\n";

make_html(1, "root",    "$root/index.html");
make_html(2, "default", "$root/default/index.html");
make_html(2, "def$big", "$root/default/index-big.html");
make_html(3, "norm",    "$root/foo.example.com/index.html");
make_html(4, "port",    "$root/foo.example.com:1234/index.html");
make_html(5, "corner",  "$root/foo.example.com/corner/index.html/index.html");
make_html(6, "bt",      "$root/foo.example.com:1234/bt.torrent");
make_html(7, "plain",   "$root/default/README");
make_html(8, "backup",  "$root/default/index.html~");
make_html(9, "welcome", "$root/default/welcome.html");
make_html(9, "welcome", "$root/default/welcome.txt");
make_html(0, "",        "$root/default/noprivs.html");
make_html(0, "privs",   "$root/default/noallprivs.html");

open(OUT,     "> $root/foo.example.com/empty") || failure("open empty: $!");
munge_mtime(44, "$root/foo.example.com/empty");

system("$ENV{SRCDIR}/gzip-r.pl --type=all $root");
munge_mtime(0, "$root/index.html.gz");
munge_mtime(0, "$root/index.html.bz2");
munge_mtime(0, "$root/default/index.html.gz");
munge_mtime(0, "$root/default/index.html.bz2");
munge_mtime(0, "$root/foo.example.com/index.html.gz");
munge_mtime(0, "$root/foo.example.com/index.html.bz2");
munge_mtime(0, "$root/foo.example.com:1234/index.html.gz");
munge_mtime(0, "$root/foo.example.com:1234/index.html.bz2");

chmod(0000, "$root/default/noprivs.html");
chmod(0600, "$root/default/noallprivs.html");

system("mkfifo $root/default/fifo");

my ($a, $b, $c, $d,
    $e, $f, $g, $h,
    $atime, $mtime) = stat("$ENV{SRCDIR}/tst/ex_cat_tst_4");
copy("$ENV{SRCDIR}/tst/ex_cat_tst_4", "$root/default/bin");
utime $atime, $mtime, "$root/default/bin";

run_tst("ex_httpd", "ex_httpd_help", "--help");
run_tst("ex_httpd", "ex_httpd_version", "--version");

my $args = "--unspecified-hostname=default --mime-types-xtra=$ENV{SRCDIR}/mime_types_extra.txt ";

my $list_pid = undef;

daemon_init("ex_httpd", $root, $args . "--virtual-hosts=true  --mmap=false --sendfile=false");
system("cat > $root/default/fifo &");
$list_pid = http_cntl_list_beg();
all_vhost_tsts();
http_cntl_list_cleanup($list_pid);
daemon_exit("ex_httpd");

$truncate_segv = 1;
daemon_init("ex_httpd", $root, $args . "--virtual-hosts=true  --mmap=true  --sendfile=false --procs=2");
system("cat > $root/default/fifo &");
$list_pid = http_cntl_list_beg();
all_vhost_tsts();
http_cntl_list_cleanup($list_pid);
daemon_exit("ex_httpd");
$truncate_segv = 0;

daemon_init("ex_httpd", $root, $args . "--virtual-hosts=true --mmap=false --sendfile=true --accept-filter-file=$ENV{SRCDIR}/tst/ex_sock_filter_out_1");
system("cat > $root/default/fifo &");
$list_pid = http_cntl_list_beg();
all_vhost_tsts();
http_cntl_list_cleanup($list_pid);
daemon_exit("ex_httpd");

my $largs = "--mime-types-xtra=$ENV{SRCDIR}/mime_types_extra.txt ";
daemon_init("ex_httpd", $root, $largs . "-d -d -d --virtual-hosts=true  --public-only=true");
all_public_only_tsts("no gen tsts");
daemon_exit("ex_httpd");

daemon_init("ex_httpd", $root, $args . "--virtual-hosts=false --pid-file=$root/abcd");
my $abcd = daemon_get_io_r("$root/abcd");
chomp $abcd;
if (daemon_pid() != $abcd) { failure("pid doesn't match pid-file"); }
all_nonvhost_tsts();
daemon_exit("ex_httpd");

my $nargs  = "--unspecified-hostname=default ";
   $nargs .= "--mime-types-main=$ENV{SRCDIR}/mime_types_extra.txt ";
   $nargs .= "--mime-types-xtra=$ENV{SRCDIR}/tst/ex_httpd_bad_mime ";
   $nargs .= "--virtual-hosts=true ";
   $nargs .= "--keep-alive=false ";
   $nargs .= "--range=false ";
   $nargs .= "--gzip-content-replacement=false ";
   $nargs .= "--error-406=false ";
   $nargs .= "--defer-accept=1 ";
   $nargs .= "--max-connections=32 ";
   $nargs .= "--max-header-sz=2048 ";
   $nargs .= "--nagle=true ";
   $nargs .= "--host=127.0.0.2 ";
   $nargs .= "--idle-timeout=16 ";
   $nargs .= "--dir-filename=welcome.html ";
   $nargs .= "--accept-filter-file=$ENV{SRCDIR}/tst/ex_httpd_null_tst_1 ";
   $nargs .= "--server-name='Apache/2.0.40 (Red Hat Linux)' ";
   $nargs .= "--canonize-host=true ";
   $nargs .= "--error-host-400=false ";

   $nargs .= "--configuration-data-jhttpd";
   $nargs .= " '(policy <default> (MIME/types-default-type bar/baz))' ";

daemon_init("ex_httpd", $root, $nargs);
tst_proc_limit(30);
$list_pid = http_cntl_list_beg();
all_none_tsts();
http_cntl_list_cleanup($list_pid);
daemon_exit("ex_httpd");

rmtree($root);

success();

END {
  my $save_exit_code = $?;
  daemon_cleanup("ex_httpd");
  $? = $save_exit_code;
}
