#! /usr/bin/perl -w

use strict;
use FileHandle;

my $name = "Vstr documentation";

my $man_header = <<'EOL';
.TH Vstr 3 "21-Mar-2002" "Vstr 0.9.1" "Vstr String Library"
.SH "SYNOPSIS"
.in \w'  'u
#include <vstr.h>
.sp
.NH
EOL

my $man_desc = <<EOL;
.ti
.HY
.SH "DESCRIPTION"
 A very simple overview is that you call vstr_init() at the start of your
program then you can create a (Vstr_base *) by calling vstr_make_base().
 You can then add/delete data from this string, using the provided functions,
if you need to use all or part of the string with a "C string" interface
then you can call vstr_export_cstr_ptr() or vstr_export_cstr_ref().
 To delete the entire vstr you call vstr_free_base() on the (Vstr_base *).
EOL

sub synopsis()
  {
    my $count = 0;
    my $args = 0;
    my $func = undef;

    sub fin
      {
	my $count = shift;
	my $args = shift;

	if ($count)
	  {
	    if (!$args)
	      {
		OUT->print("void");
	      }
	    OUT->print(");\n");
	  }
      }

    while (<IN>)
      {
	if (s!^(Constant|Function|Member): (.*)\(\)$!$2!)
	  {
	    chomp;
	    fin($count, $args);
	    ++$count;
	    $args = 0;
	    $func = $_;
	  }
	elsif (/^ Type: (.*)/)
	  {
	    my $spc = " ";

	    $_ = $1;
	    chomp;

	    if (/\*$/)
	      { $spc = ""; }

	    OUT->print(".br\n"); # FIXME: \w doesn't account for space filling
	    OUT->print(".in \\w'  $_$spc\\fB$func\\fR('u\n");
	    OUT->print(".ti \\w'  'u\n");
	    OUT->print("$_$spc\\fB$func\\fR(");
	  }
	elsif (/^ Type\[.+\]: (.*)/)
	  {
	    $_ = $1;
	    chomp;
	    if ($args)
	      {
		OUT->print(", ");
	      }

	    ++$args;
	    OUT->print("$1");
	  }
	elsif (/^Section:/)
	  {
	    fin($count, $args);
	    $count = 0;
	    OUT->print(".sp\n");
	  }
      }

    fin($count, $args);
    OUT->print("\n");
  }

sub convert()
  {
    my $in_pre_tag = "";
    my $in_const = 0;

    while (<IN>)
      {
	my $next_in_const = 0;

	my $beg_replace = ".br\n";

	if ($in_const)
	  {
	    $beg_replace = "\n";
	  }

	if (s!^(Constant|Function|Member): (.*)$!$beg_replace\\fB$1: \\fR $2! ||
	    s!^ Explanation:\s*$!$beg_replace\\fBExplanation:\\fR! ||
	    s!^ Note:\s*$!.sp\n\\fBNote:\\fR! ||
	    s!^Section:\s*(.*)$!.SH $1! ||
	    0)
	  {
	    if (defined ($1) && ($1 eq "Function"))
              {
	        $_ = ".ti -2\n" . $_;
              }
	    if (defined ($1) && ($1 eq "Constant"))
	      {
		$next_in_const = 1;
	      }
	  }
	elsif (m!^ ([A-Z][a-z]+)(\[\d\]|\[ \.\.\. \])?: (.*)$!)
	  {
	    if (defined $2)
	      {
		if ($1 eq "Type")
		  {
		    $_ = ".br\n$1\\fB$2\\fR: $3\n";
		  }
		else
		  {
		    $_ = ".br\n$1\\fB$2\\fR: $3\n";
		  }
	      }
	    else
	      {
		if ($1 eq "Type")
		  {
		    $_ = ".br\n$1: $3\n";
		  }
		else
		  {
		    $_ = ".br\n$1: $3\n";
		  }
	      }
	  }
	elsif (/^ \.\.\./)
	  {
	    if (/\.\.\.$/)
	      {
		$_ = ".Ve\n$_.Vb 4\n";
		$in_pre_tag = "</pre>";
	      }
	    else 
	      {
		$_ = ".Ve\n$_";
		$in_pre_tag = "";
	      }
	  }
	elsif (/\.\.\.$/)
	  {
	    $_ = "$_\n.Vb 4";
	    $in_pre_tag = "\n.Ve";
	  }
	elsif (!$in_pre_tag)
	  {
	    if (!/^$/)
	      {
		chomp;
		if (/^  /)
		  {
		    $_ = "\n.br\n" . $_;
		  }
	      }
	  }
	
	$in_const = $next_in_const;

	OUT->print($_);
      }
  }

if (!open (IN, "<functions.txt"))
  {
    die "Open (read): $@";
  }

if (!open (OUT, ">functions.3"))
  {
    die "Open (write): $@";
  }

OUT->print($man_header);

synopsis();

if (!open (IN, "<functions.txt"))
  {
    die "Open (read): $@";
  }

OUT->print($man_desc);

convert();

exit (0);
