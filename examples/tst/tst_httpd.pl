#! /usr/bin/perl -w

use strict;
use File::Path;

use IO::Socket;

push @INC, "$ENV{SRCDIR}/tst";
require 'vstr_tst_examples.pl';

sub sub_http_tst
  {
    my $io_r = shift;
    my $io_w = shift;
    my $xtra = shift;
    my $sz   = shift;

    my $sock = new IO::Socket::INET->new(PeerAddr => daemon_addr(),
					 PeerPort => daemon_port(),
					 Proto    => "tcp",
					 Type     => SOCK_STREAM,
					 Timeout  => 2) || failure("connect");

    my $data_r = "";
    my $data_w = "";
    { local ($/);
      open(INFO, "< $io_r") || failure("open $io_r: $!");
      $data_r = <INFO>;
      close(INFO);
    }
    $data_r =~ s/\n/\r\n/g;

    my $len_r = length($data_r);
    my $off_r = 0;

    while (1)
      {
	if ($len_r)
	  {
	    my $len = $sock->syswrite($data_r, $len_r, $off_r);
	    $len_r -= $len; $off_r += $len;
	  }

	$sock->sysread(my($buff), 4096);
	last if ( !$buff);
	$data_w .= $buff;
      }

    # Remove date, because that changes each time
    $data_w =~ s/^(Date:).*$/$1/gm;
    # Remove last-modified = start date for error messages
    $data_w =~
      s!(HTTP/1[.]1 \s (?:400|403|404|405|412|414|416|417|501|505) .*)$ (\n)
	^(Date:)$ (\n)
	^(Server:.*)$ (\n)
	^(Last-Modified:) .*$
	!$1$2$3$4$5$6$7!gmx;

    open(OUT, "> $io_w") || failure("open $io_w: $!");
    print OUT $data_w;
    close(OUT);
  }

my $root = "ex_httpd_root";
my $args = $root;

mkpath([$root . "/default",
	$root . "/foo.example.com",
	$root . "/foo.example.com:1234"]);

daemon_init("ex_httpd", $root);

sub make_index_html
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

    my ($a, $b, $c, $d,
	$e, $f, $g, $h,
	$atime, $mtime) = stat("$ENV{SRCDIR}/tst/ex_httpd_tst_1");
    $atime -= ($num * (60 * 60 * 24));
    $mtime -= ($num * (60 * 60 * 24));
    utime $atime, $mtime, $fname;
  }

make_index_html(1, "root",    "$root/index.html");
make_index_html(2, "default", "$root/default/index.html");
make_index_html(3, "norm",    "$root/foo.example.com/index.html");
make_index_html(4, "port",    "$root/foo.example.com:1234/index.html");

sub_tst(\&sub_http_tst, "ex_httpd");

daemon_exit("ex_httpd");

rmtree($root);

success();

END {
  my $save_exit_code = $?;
  daemon_cleanup("ex_httpd");
  $? = $save_exit_code;
}
