#! /usr/bin/perl -w

if (@ARGV < 2) { die " Format: $0 <HTML> <CSS>"; }

open(HTML, "< $ARGV[0]") || die "open($ARGV[0]): $!";
open(CSS,  "< $ARGV[1]") || die "open($ARGV[1]): $!";

my %css2style = ();

while (<CSS>)
{
	/^ \s+ pre[.]c2html # Everything starts as a sub of pre
            (?:\s+ span[.]([a-z0-9]+))?  # If it's not pre, find the span class.
           \s+ { \s ([^}]+) \s } $/x || # Find the style fino.
	next;

	my $class = $1;
	my $style = $2;

	if (! defined ($class)) { $class = "c2html"; }

	$css2style{$class} = $style;
}


while (<HTML>)
{
	s/class="([^"]+)"/style="$css2style{$1}"/g;
	print;
}

