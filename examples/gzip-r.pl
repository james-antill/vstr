#! /usr/bin/perl -w

use strict;

use File::Find;
use File::Temp qw/tempfile/;

use FileHandle;

# It's there on FC1, but not on RHEL3
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

use Getopt::Long;
use Pod::Usage;

my $man = 0;
my $help = 0;

my $force_compress = 0;
my $verbose_compress = 0;

pod2usage(0) if !
GetOptions ("force!"   => \$force_compress,
	    "verbose+" => \$verbose_compress,
	    "help|?"   => \$help,
	    "man"      => \$man);
pod2usage(-exitstatus => 0, -verbose => 1) if $help;
pod2usage(-exitstatus => 0, -verbose => 2) if $man;


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
	    print STDOUT "Symlink: $name => $dst\n" if ($verbose_compress > 1);
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

    print STDOUT "Compress: $name\n" if ($verbose_compress > 0);

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

__END__

=head1 NAME

gzip-r.pl - Recursive "intelligent" gzip

=head1 SYNOPSIS

gzip-r.pl [options] [dirs|files ...]

 Options:
  --help -?         brief help message
  --man             full documentation
  --force           force recompression
  --verbose         print filenames

=head1 OPTIONS

=over 8

=item B<--help>

Print a brief help message and exits.

=item B<--man>

Prints the manual page and exits.

=item B<--force>

Recompresses .gz files even when they are newer than their source.

=item B<--verbose>

Prints the name of each file being compressed followed by a newline, if
specified once. If specified more than once also prints the name of each symlink
created.

=back


=head1 DESCRIPTION

B<gzip-r.pl> will take all files from the directories and filenames passed
as fname. If the extensions of the files are not likely to be compressible
(Ie. .gz, .bz2, .rpm, .zip) or are tmp files (Ie. .tmp, ~, #) then they are
skipped.
 If the fname is a regular file a fname.gz output file will be generated
(without removing the input fname file).
 If the fname is a symlink a fname.gz symlink pointing to the target of fname
with a .gz extension added will be created.

 gzip is called with the options: --no-name --best

=cut
