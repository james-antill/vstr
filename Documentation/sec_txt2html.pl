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

my $title = "Classification of Red Hat security alerts 2003";

my $html_header = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML//EN\">\n<html>\n<head>\n";
my $html_body = <<EOF;
    <style type="text/css">
      A:visited { color: #ff4040; }
      A:hover { color: #20b2aa; }

      P { text-indent: 1cm; }

      body { background: #FFF; }

  tr.rh { font-weight: bold; font-size: 150%;
          background: #DDD; }

  tr.r10                   { background: #fff; }
  tr.r10:hover             { background: #bbb; }
  tr.r20                   { background: #eee; }
  tr.r20:hover             { background: #bbb; }
  tr.r11                   { background: #fff; text-decoration: underline; }
  tr.r11:hover             { background: #bbb; text-decoration: underline; }
  tr.r21                   { background: #eee; text-decoration: underline; }
  tr.r21:hover             { background: #bbb; text-decoration: underline; }


      strong.sapi  { color: #F00; background: white; }
      strong.msapi { color: #00F; background: white; }

    </style>

  </head>
  <body>
EOF

my $html_footer = "\n</body></html>";


if (!open (OUT, "> security_problems.html"))
  {
    die "Open (write): $@";
  }


print OUT $html_header;
print OUT "<title>", $title, "</title>", "\n";
print OUT $html_body;
print OUT "<h1>", $title, "</h1>", "\n";

print OUT <<EOL;
 <p> A <strong class="sapi">RED</strong> error indicates an error
that <b>could not have occured</b> if the program had been using a
<a href="comparison.html">real
dynamic string API</a>. </p>
 <p> A <strong class="msapi">BLUE</strong> error indicates an error
that <b>could not have occured</b> it the program had been using
<a href"overview.html">Vstr</a>. </p>
EOL

print OUT "<table width=\"100%\">";
print OUT "<tr class=\"rh\"><td>" . "Package name";
print OUT "</td><td>" . "Red Hat Advisory ID";
print OUT "</td><td>" . "Types of error";
print OUT "</td></tr>\n";

if (!open (IN, "< $docs/security_problems.txt"))
  {
    die "Open (read): $@";
  }

my $name = '';
my $ref = {rh => ''};

my @t = ();

my $t_out =
  {
   OBO => '<strong class="sapi">Off By One</strong><br>',
   BO => '<a href="http://www.dwheeler.com/secure-programs/Secure-Programs-HOWTO.html#BUFFER-OVERFLOW"><strong class="sapi">Buffer Overflow</strong></a><br>',
   IO => '<strong class="sapi">Integer Overflow</strong><br>',
   XSS => 'Cross Site Scripting<br>',
   IE => 'Improper Encoding<br>',
   VSTRIE => '<strong class="msapi">Improper Encoding</strong><br>',
   IV => 'Input Validation<br>',
   VSTRIV => '<strong class="msapi">Input Validation</strong><br>',
   DF => 'Double Free<br>',
   FU => 'Free memory that is un use<br>',
   VSTRFU => '<strong class="msapi">Free memory that is in use</strong><br>',
   TF => 'Temporary File Creation<br>',
   IL => 'Information Leak<br>',
  };
my $t_sapi =
  {
   OBO => 1,
   BO => 1,
   IO => 1,
   XSS => 0,
   VSTRIE => -1,
   IE => 0,
   IV => 0,
   VSTRIV => -1,
   DF => 0,
   FU => 0,
   VSTRFU => -1,
   TF => 0,
   IL => 0,
  };

my $row = 0;

my $cnt_stup = 0;
my $cnt_vstr = 0;
my $cnt_dstr = 0;

while (<IN>)
  {
    if (0) {}
    elsif (/^(Name): (.*)$/)
      {
	@t = ();
	$name = $2;
      }
    elsif (/^(RH-ref): (.*)$/)
      { $ref->{rh} = $2; }
    elsif (/^(Type): (.*)$/)
      {
	push @t, $2;
      }
    elsif (/^$/)
      {
	# Output info...

	$row = ($row % 2) + 1;
	my $stupid_error = 1;
	for (@t)
	  {
	    if (!defined($t_sapi->{$_}) || !$t_sapi->{$_})
	      {
		$stupid_error = 0;
		last;
	      }
	    if (!defined($t_sapi->{$_}) || ($t_sapi->{$_} == -1))
	      {
		$stupid_error = -1;
	      }
	  }

print OUT "<tr class=\"r${row}${stupid_error}\"><td>";
if (0) {}
elsif ($stupid_error ==  1)
  {
    ++$cnt_stup;
    print OUT '<strong class="sapi">' .  $name . '</strong>';
  }
elsif ($stupid_error == -1)
  {
    ++$cnt_stup;
    ++$cnt_dstr;
    print OUT '<strong class="msapi">' . $name . '</strong>';
  }
elsif ($stupid_error ==  0)
  {
    ++$cnt_stup;
    ++$cnt_dstr;
    ++$cnt_vstr;
    print OUT $name;
  }

$_ = $ref->{rh}; s/(\d{4}):(\d{3})-\d{2}/$1-$2/;
print OUT "</td><td><a href=\"http://rhn.redhat.com/errata/" . $_ . '.html';
print OUT "\">" . $ref->{rh};
print OUT "</a>";
print OUT "</td><td>";
	for (@t)
	  {
	    if (!defined ($t_out->{$_}))
	      {
		OUT->print($_ . '<BR>');
		next;
	      }
	    OUT->print($t_out->{$_});
	  }
print OUT "</td></tr>\n";
      }
  }

my $cnt_p_vstr = sprintf("%d%%", (($cnt_vstr * 100) / $cnt_stup));
my $cnt_p_dstr = sprintf("%d%%", (($cnt_dstr * 100) / $cnt_stup));
print OUT <<EOL;
</table>
<p> Note that this means that of the <b>$cnt_stup</b> security flaws,
 only <strong class="msapi">$cnt_vstr ($cnt_p_vstr)</strong> would have happened if all
the packages had used <a href="overview.html">Vstr</a> and only
 <strong class="sapi">$cnt_dstr ($cnt_p_dstr)</strong> if they\'d at least
used ay real dynamic string API.</p>
EOL

print OUT $html_footer;
