#! /usr/bin/perl -w

use strict;

use File::Find;
use File::Temp qw/tempfile/;

use FileHandle;

my $force_compress = 0;

my $have_perl_cmpstat = 0;

#  Might want to add .iso or some .mov type exts ... however non-trivial savings
# are often on those files.
my $filter_exts_re = qr/(?:
			[.]gz  |
			[.]bz2 |
			[.]rpm |
			[.]zip |
			[.]tmp |
			~      |
			\#
		       )$/x;

sub filter_no_gzip
  { # Don't compress gzip files...
    grep(!/$filter_exts_re/,  @_)
  }

our $out;
our $fname;

sub gzip_file
  {
    my $name   = $_;
    my $namegz = $_ . ".gz";

    if (-l $name && -f $name)
      { # deal with symlinks...
	my $dst = readlink $name;

	defined($dst) || die "Can't readlink $name: $!";

	if ($dst !~ /$filter_exts_re/)
	  {
	    unlink($namegz);
	    symlink($dst . ".gz", $namegz) || die "Can't symlink($namegz): $!";;
	  }
	return;
      }

    if (! -f _)
      {
	return;
      }

    if (!$force_compress)
      {
	my @st_name   = stat _;
	if (-f $namegz)
	  { # If .gz file is already newer, skip it...
	    my @st_namegz = stat _;
	if ($st_name[9] < $st_namegz[9])
	  { return; }
	  }
      }

    ($out, $fname) = tempfile("gzip-r.XXXXXXXX", SUFFIX => ".tmp");

    defined ($out) || die("Can't create tempfile: $!");

    open(IN, "-|", "gzip", "--to-stdout", "--no-name", "--best", "--", $name) ||
      die("Can't gzip: $!");

    my $bs = 1024 * 8;
    $/ = \$bs;

    while (<IN>) { $out->print($_); }

    close(IN)   || die "Failed closing input: $!";

    rename($fname, $namegz)             || die "Can't rename($namegz): $!";
    if ($have_perl_cmpstat) {
    File::Temp::cmpstat($out, $namegz)  || die "File moved $namegz: $!";
    }
    close($out)                         || die "Failed closing output: $!";
    # No stupid fchmod, Grr....
    chmod(0644, $namegz);
  }

find({ preprocess => \&filter_no_gzip, wanted => \&gzip_file }, @ARGV);

$out = undef;

END {
  if (defined($out))
    {
      File::Temp::unlink0($out, $fname) || die "Can't unlink($fname): $!"; $?;
    }
}
