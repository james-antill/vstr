#! /usr/bin/perl -w

use strict;
use FileHandle;

my $docs = undef;

if (0) {}
elsif (-x "../configure") # In docs dir...
  {
    $docs ="../Documentation";
  }
elsif (-x "../../configure") # in build subdir
  {
    $docs ="../../Documentation";
  }

if (!defined ($docs))
  {
    STDERR->print("Can't find configure.\n");
    exit (1);
  }

my $name = "Vstr documentation";

my $html_header = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n<html>\n<head>\n";
my $html_body = <<EOF;
    <style type="text/css">
      A:visited { color: #ff4040; }
      A:hover { color: #20b2aa; }

      P { text-indent: 1cm; }

      body { background: #FFFFFF; }

      td.heading { background: #DDDDDD; }
    </style>

  </head>
  <body>
EOF

my $html_footer = "\n</td></tr></table>\n</body></html>";

sub convert_index()
  {
    my $in_pre_tag = "";

    OUT->print("</td></tr></table><table width=\"90%\"><tr><td>\n");
    OUT->print("<h3>Index of sections</h3>\n");
    OUT->print("<ul>\n");

    my $done = 0;

    while (<IN>)
      {
	if (/^(Constant|Function|Member): (.*)$/)
	  {
	    my $name = $2;
	    my $uri = $2;
	    $uri =~ s/([^[:alnum:]:_])/sprintf("%%%02x", ord($1))/eg;

	    print OUT "<li><a href=\"#$uri\">$name</a>\n";
	  }
        elsif (/^Section:\s*(.*)$/)
	  {
	    my $section = $1;
	    my $uri = $1;
	    $uri =~ s/([^[:alnum:]:_])/sprintf("%%%02x", ord($1))/eg;

	    if ($done)
	      { print OUT "</ul>"; }
	    $done = 1;

	    print OUT "<li><a href=\"#$uri\">$section</a>\n<ul>\n";
	  }
      }

    OUT->print("</ul></ul>\n");
  }

sub conv_html
{
  my $text = shift;

  if (defined ($text)) { $_ = $text; }

s/&/&amp;/g;        # must remember to do this one first!
s/</&lt;/g;         # this is the most important one
s/>/&gt;/g;         # don't close too early
s/"/&quot;/g;       # only in embedded tags, i guess

  return ($_);
}

sub convert()
  {
    my $in_pre_tag = "";
    my $in_const = 0;

    while (<IN>)
      {
	my $next_in_const = 0;

	my $beg_replace = <<EOL;
</td></tr></table><table width="80%"><tr><td bgcolor="#DDDDDD">
EOL

	if ($in_const)
	  {
	    $beg_replace = qw(<br>);
	  }

	if (s!^(Constant|Function|Member): (.*)$!$beg_replace<strong>$1: </strong> $2! ||
	    s!^ Explanation:\s*$!</td></tr><tr><td><strong>Explanation:</strong></td></tr><tr><td><p>! ||
	    s!^ Note:\s*$!</td></tr><tr><td><strong>Note:</strong></td></tr><tr><td><p>! ||
	    s!^Section:\s*(.*)$!</td></tr></table><table width="90%"><tr><td bgcolor="#DDDDFF"><h2>$1</h2>! ||
	    0)
	  {
	    if (defined ($1))
            {
              if ($1 eq "Constant")
	      {
                my $uri = $2;
                $uri =~ s/([^[:alnum:]:_])/sprintf("%%%02x", ord($1))/eg;

		$next_in_const = 1;

                $_ = "<a name=\"$uri\">" . $_ . "</a>";
	      }
              elsif ($1 eq "Function")
              {
                my $uri = $2;
                $uri =~ s/([^[:alnum:]:_])/sprintf("%%%02x", ord($1))/eg;

                $_ = "<a name=\"$uri\">" . $_ . "</a>";
              }
              elsif ($1 eq "Member")
              { }
              else
              { # Section...
                my $uri = $1;
                $uri =~ s/([^[:alnum:]:_])/sprintf("%%%02x", ord($1))/eg;

                s/<h2>/<a name="$uri"><h2>/;
                $_ .= "</a>";
              }
            }
	  }
	elsif (m!^ ([A-Z][a-z]+)(\[\d\]|\[ \.\.\. \])?: (.*)$!)
	  {
            my $attr = $1;
            my $param_num = $2;
            my $text = $3;

            $text = conv_html($text);
            conv_html();
	    if (defined $param_num)
	      {
		if ($attr eq "Type")
		  {
		    $_ = "<br>$attr<strong>$param_num</strong>: $text";
		  }
		else
		  {
		    $_ = "</td></tr><tr><td>$attr<strong>$param_num</strong>: $text";
		  }
	      }
	    else
	      {
		if ($attr eq "Type" || $attr eq "Returns")
		  {
		    $_ = "<br>$attr: $text";
		  }
		else
		  {
		    $_ = "</td></tr><tr><td>$attr: $text";
		  }
	      }
	  }
	elsif (/^ \.\.\./)
	  {
	    if (/\.\.\.$/)
	      {
		$_ = "</pre>$_<br><pre>";
		$in_pre_tag = "</pre>";
	      }
	    else 
	      {
		$_ = "</pre>$_";
		$in_pre_tag = "";
	      }
	  }
	elsif (/\.\.\.$/)
	  {
            conv_html();
	    $_ = "$_<br><pre>";
	    $in_pre_tag = "</pre>";
	  }
	elsif (!$in_pre_tag && /^  /)
	  {
            conv_html();
	    $_ = "</p><p>$_";
	  }
	
	$in_const = $next_in_const;

	print OUT;
      }
  }

if (!open (OUT, "> functions.html"))
  {
    die "Open (write): $@";
  }

print OUT $html_header;
print OUT "<title>", "$name -- functions", "</title>", "\n";
print OUT $html_body;
print OUT "<table width=\"100%\"><tr><td bgcolor=\"#DDFFDD\">", 
  "<h1>", "$name -- functions", "</h1>", "\n";

if (!open (IN, "< $docs/functions.txt"))
  {
    die "Open (read): $@";
  }
convert_index();
if (!open (IN, "< $docs/functions.txt"))
  {
    die "Open (read): $@";
  }
convert();

print OUT $html_footer;

if (!open (OUT, "> constants.html"))
  {
    die "Open (write): $@";
  }

print OUT $html_header;
print OUT "<title>", "$name -- constants", "</title>", "\n";
print OUT $html_body;
print OUT "<table width=\"100%\"><tr><td bgcolor=\"#DDFFDD\">", 
  "<h1>", "$name -- constants", "</h1>", "\n";

if (!open (IN, "< $docs/constants.txt"))
  {
    die "Open (read): $@";
  }
convert_index();
if (!open (IN, "< $docs/constants.txt"))
  {
    die "Open (read): $@";
  }
convert();

print OUT $html_footer;

if (-r "structs.txt")
  {
    if (!open (OUT, "> $docs/structs.html"))
      {
	die "Open (write): $@";
      }
    
    print OUT $html_header;
    print OUT "<title>", "$name -- structs", "</title>", "\n";
    print OUT $html_body;
    print OUT "<table width=\"100%\"><tr><td bgcolor=\"#DDFFDD\">", 
      "<h1>", "$name -- structs", "</h1>", "\n";
    
    if (!open (IN, "< structs.txt"))
      {
	die "Open (read): $@";
      }
    convert_index();
    if (!open (IN, "< $docs/structs.txt"))
      {
	die "Open (read): $@";
      }
    convert();
    
    print OUT $html_footer;
  }

exit 0;
