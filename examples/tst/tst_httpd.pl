#! /usr/bin/perl -w

use strict;
use File::Path;
use File::Copy;

push @INC, "$ENV{SRCDIR}/tst";
require 'vstr_tst_examples.pl';

sub httpd__munge_ret
  {
    my $output = shift;

    # Remove date, because that changes each time
    $output =~ s/^(Date:).*$/$1/gm;
    # Remove last-modified = start date for error messages
    $output =~
      s!(HTTP/1[.]1 \s (?:301|40[0345]|41[2467]|50[015]) .*)$ (\n)
	^(Date:)$ (\n)
	^(Server:.*)$ (\n)
	^(Last-Modified:) .*$
	!$1$2$3$4$5$6$7!gmx;
    # Remove last modified for trace ops
    $output =~
      s!^(Last-Modified:).*$ (\n)
        ^(Accept-Ranges:.*)$ (\n)
        ^(Content-Type: \s message/http.*)$
	!$1$2$3$4$5!gmx;

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
			   $xtra->{shutdown_w}, $xtra->{slow_write});

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

    $data = $xtra->{gen_input}->();

    my $output = daemon_io($sock, $data,
			   $xtra->{shutdown_w}, $xtra->{slow_write}, 1);

    $output = httpd__munge_ret($output);
    daemon_put_io_w($io_w, $output);
  }

sub gen_tsts()
  {
    my $gen_cb = sub {
      my $data = ("\r\n" x 200_000) . ("x" x 500_000);
      return $data;
    };

#    sub_tst(\&httpd_gen_tst, "ex_httpd_null",
#	    {gen_input => $gen_cb});
    sub_tst(\&httpd_gen_tst, "ex_httpd_null",
	    {gen_input => $gen_cb, shutdown_w => 0});
#    sub_tst(\&httpd_gen_tst, "ex_httpd_null",
#	    {gen_input => $gen_cb,                  slow_write => 1});
#    sub_tst(\&httpd_gen_tst, "ex_httpd_null",
#	    {gen_input => $gen_cb, shutdown_w => 0, slow_write => 1});
  }

sub all_vhost_tsts()
  {
    sub_tst(\&httpd_file_tst, "ex_httpd");
    sub_tst(\&httpd_file_tst, "ex_httpd_errs");

    sub_tst(\&httpd_file_tst, "ex_httpd",
	    {shutdown_w => 0});
    sub_tst(\&httpd_file_tst, "ex_httpd_errs",
	    {shutdown_w => 0});
    sub_tst(\&httpd_file_tst, "ex_httpd_shut",
	    {shutdown_w => 0});

    sub_tst(\&httpd_file_tst, "ex_httpd",
	    {                 slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_errs",
	    {                 slow_write => 1});

    sub_tst(\&httpd_file_tst, "ex_httpd",
	    {shutdown_w => 0, slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_errs",
	    {shutdown_w => 0, slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_shut",
	    {shutdown_w => 0, slow_write => 1});
    gen_tsts();
  }

sub all_nonvhost_tsts()
  {
    sub_tst(\&httpd_file_tst, "ex_httpd_non-virtual-hosts");
    sub_tst(\&httpd_file_tst, "ex_httpd_non-virtual-hosts",
	    {shutdown_w => 0});
    sub_tst(\&httpd_file_tst, "ex_httpd_non-virtual-hosts",
	    {                 slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_non-virtual-hosts",
	    {shutdown_w => 0, slow_write => 1});
    gen_tsts();
  }

sub all_public_only_tsts()
  {
    sub_tst(\&httpd_file_tst, "ex_httpd_public-only");
    sub_tst(\&httpd_file_tst, "ex_httpd_public-only",
	    {shutdown_w => 0});
    sub_tst(\&httpd_file_tst, "ex_httpd_public-only",
	    {                 slow_write => 1});
    sub_tst(\&httpd_file_tst, "ex_httpd_public-only",
	    {shutdown_w => 0, slow_write => 1});
    gen_tsts();
  }

if (@ARGV)
  {
    daemon_status(shift);
    if (@ARGV && ($ARGV[0] eq "non-virtual-hosts"))
      {
	all_nonvhost_tsts();
      }
    else
      {
	all_vhost_tsts();
      }
    success();
  }

my $root = "ex_httpd_root";
my $args = $root;

rmtree($root);
mkpath([$root . "/default",
	$root . "/foo.example.com/nxt",
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

make_html(1, "root",    "$root/index.html");
make_html(2, "default", "$root/default/index.html");
make_html(3, "norm",    "$root/foo.example.com/index.html");
make_html(4, "port",    "$root/foo.example.com:1234/index.html");
make_html(0, "",        "$root/default/noprivs.html");
make_html(0, "privs",   "$root/default/noallprivs.html");

system("$ENV{SRCDIR}/gzip-r.pl $root");
munge_mtime(0, "$root/index.html.gz");
munge_mtime(0, "$root/default/index.html.gz");
munge_mtime(0, "$root/foo.example.com/index.html.gz");
munge_mtime(0, "$root/foo.example.com:1234/index.html.gz");

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

daemon_init("ex_httpd", $root, "--virtual-hosts=true  --mmap=false --sendfile=false");
system("cat > $root/default/fifo &");
all_vhost_tsts();
daemon_exit("ex_httpd");

daemon_init("ex_httpd", $root, "--virtual-hosts=true  --mmap=true  --sendfile=false");
system("cat > $root/default/fifo &");
all_vhost_tsts();
daemon_exit("ex_httpd");

daemon_init("ex_httpd", $root, "--virtual-hosts=true --mmap=true  --sendfile=true --accept-filter-file=$ENV{SRCDIR}/tst/ex_sock_filter_out_1");
system("cat > $root/default/fifo &");
all_vhost_tsts();
daemon_exit("ex_httpd");

daemon_init("ex_httpd", $root, "--virtual-hosts=true  --public-only");
all_public_only_tsts();
daemon_exit("ex_httpd");

daemon_init("ex_httpd", $root, "--virtual-hosts=false --pid-file=$root/abcd");
my $abcd = daemon_get_io_r("$root/abcd");
chomp $abcd;
if (daemon_pid() != $abcd) { failure("pid doesn't match pid-file"); }
all_nonvhost_tsts();
daemon_exit("ex_httpd");

rmtree($root);

success();

END {
  my $save_exit_code = $?;
  daemon_cleanup("ex_httpd");
  $? = $save_exit_code;
}
