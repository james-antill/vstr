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
my $chown_compress = 0;
my $verbose_compress = 0;
my $type_compress = "gzip";

pod2usage(0) if !
GetOptions ("force!"   => \$force_compress,
	    "chown!"   => \$chown_compress,
	    "type|t=s" => \$type_compress,
	    "verbose+" => \$verbose_compress,
	    "help|?"   => \$help,
	    "man"      => \$man);
pod2usage(-exitstatus => 0, -verbose => 1) if $help;
pod2usage(-exitstatus => 0, -verbose => 2) if $man;


my $ext_compress = ".gz";
my @cmd_compress_args = ("gzip", "--to-stdout", "--no-name", "--best");

if ($type_compress eq "bzip2")
  {
    $ext_compress = ".bz2";
    @cmd_compress_args = ("bzip2", "--stdout", "--best");
  }
elsif ($type_compress ne "gzip")
  { pod2usage(-exitstatus => 1); }

sub filter_no_gzip
  { # Don't compress compressed files...
    grep(!/$filter_exts_re/,  @_)
  }

our $out;
our $fname;

sub gzip_file
  {
    my $name   = $_;
    my $namegz = $_ . $ext_compress;

    if (-l $name && -f $name)
      { # deal with symlinks...
	my $dst = readlink $name;

	defined($dst) || die "Can't readlink $name: $!";

	my $dst_gz = $dst . $ext_compress;
	if (($dst !~ /$filter_exts_re/) && -f $dst_gz)
	  {
	    unlink($namegz);
	    print STDOUT "Symlink: $name => $dst\n" if ($verbose_compress > 1);
	    symlink($dst_gz, $namegz) || die "Can't symlink($namegz): $!";
	  }
	return;
      }

    if (! -f _)
      {
	return;
      }

    my @st_name   = stat _;
    if (!$force_compress)
      {
	if (-f $namegz)
	  { # If .gz file is already newer, skip it...
	    my @st_namegz = stat _;

	    if ($st_name[9] < $st_namegz[9])
	      { return; }
	  }
      }

    ($out, $fname) = tempfile("gzip-r.XXXXXXXX", SUFFIX => ".tmp");

    defined ($out) || die("Can't create tempfile: $!");
    binmode $out;

    print STDOUT "Compress: $name\n" if ($verbose_compress > 0);

    open(IN, "-|", @cmd_compress_args, "--", $name) || 
      die("Can't $type_compress: $!");
    binmode IN;

    my $bs = 1024 * 8; # Do IO in 8k blocks
    $/ = \$bs;

    while (<IN>) { $out->print($_); }

    # If the the gzip file is 95% of the original, delete it
    $out->autoflush(1);
    if (-s $out >= (($st_name[7] * 95) / 100))
      {
	close(IN);
	close($out);
	unlink($fname);
	return;
      }

    close(IN)   || die "Failed closing input: $!";

    rename($fname, $namegz)             || die "Can't rename($namegz): $!";
    if ($have_perl_cmpstat) {
    File::Temp::cmpstat($out, $namegz)  || die "File moved $namegz: $!";
    }
    close($out)                         || die "Failed closing output: $!";
    # No stupid fchmod/fchown in perl, Grr....
    chmod($st_name[2] & 0777, $namegz);
    if ($chown_compress)
      { chown($st_name[4], $st_name[5], $namegz); }
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

gzip-r.pl - Recursive "intelligent" gzip/bzip2

=head1 SYNOPSIS

gzip-r.pl [options] [dirs|files ...]

 Options:
  --help -?         brief help message
  --man             full documentation
  --force           force recompression
  --verbose         print filenames
  --chown           chown gzip files
  --type -t         type of compression files

=head1 OPTIONS

=over 8

=item B<--help>

Print a brief help message and exits.

=item B<--man>

Prints the manual page and exits.

=item B<--force>

Recompresses .gz files even when they are newer than their source.

=item B<--chown>

Make the gzip output files have the same owner as the input files.

=item B<--type>

Make the compression type either gzip or bzip2.

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
