#! /usr/bin/perl -w

use strict;

# Using: http://httpd.apache.org/docs/logs.html

# apache-httpd default formats...

# %h %l %u %t "%r" %>s %b "%{Referer}i" "%{User-Agent}i" <combined>
# %h %l %u %t "%r %>s %b                                 <common>
# %{Referer}i -> %U                                      <referer>
# %{User-agent}i                                         <agent>

# %h <host ip/resolved name>
# %l <rfc1413 ident>
# %u <HTTP auth userid>
# %t [10/Oct/2000:13:55:36 -0700]
#    [day/month/year:hour:minute:second zone]
# %r <http request line>
# %>s <status code>
# %b <request object size>
# %{Referer}i <referer header from request>
# %{User-Agent}i <user-agent header from request>
# % %U - The URL path requested, not including any query string.

while (<>)
  {
    next unless(/^
		 (\w+) \s         # Month
		 (\d+) \s         # Day
		 (\d+:\d+:\d+) \s # Time
		 \w+ \s           # Logging host
		 jhttpd           # It's the jhttpd server
		 \[ \d+ \]        # Pid of web server
		 : \s             # MSG Seperator...
		 (.+)
		 $/x);
    my ($mon, $day, $tm, $msg) = ($1, $2, $3, $4);
    my $year = undef;
    my $off = "-0500";

    ($_,$_,$_,$_,$_,$year,$_,$_) = localtime(time);

    $year += 1900;

    $_ = $msg;
    if (0) {}
    elsif (/^REQ \s (GET|HEAD) \s
	    from \[ (\d+.\d+.\d+.\d+) [@] \d+ \] \s # IP
	    ret  \[ (\d+) \s [^]]+ \] \s            # return code
	    sz   \[ [^:]+ : (\d+) \] \s             # Size
	    host \[ ".*?" \] \s                       # Host header
	    UA   \[ "(.*?)" \] \s                     # User-agent header
	    ref  \[ "(.*?)" \] \s                     # Referer (sic) header
	    ver  \[ "(HTTP[^]]+?)" \]                 # HTTP version
	    : \s                                    # MSG Seperator...
	    (.+)                                    # URL path
	    $/x)
      {
	my $ip      = $2;
	my $req     = $1 . ' ' . $8 . ' ' . $7;
	my $ret     = $3;
	my $sz      = $4;
	my $referer = length($6) ? '"' . $6 . '"' : '-';
	my $ua      = length($5) ? '"' . $5 . '"' : '-';

	print <<EOL;
$ip - - [$day/$mon/$year:$tm $off] "$req" $ret $sz $referer $ua
EOL
      }
    elsif (/^ERREQ \s from/)
      {
      }
  }
