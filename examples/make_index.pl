#! /usr/bin/perl -w

use strict;

use File::Temp qw/tempfile/;

use FileHandle;

my $prefix = "j";

if (-f "./ex_dir_list")
  { $prefix = "./ex_"; }

my $dir_name  = undef;

use Getopt::Long;
use Pod::Usage;

my $man = 0;
my $help = 0;

my $filter_def_args = "-A .. --deny-name-beg . -D index.html -D dir_list.css --deny-name-end .gz --deny-name-end .tmp --deny-name-end '~' --deny-name-end '#'";
my $filter_args = $filter_def_args;

my $sort_def_args = "--without-locale";
my $sort_args = $sort_def_args;
my $html_def_args = "";
my $html_args = $html_def_args;

pod2usage(0) if !
GetOptions ("filter-args=s@" => \$filter_args,
	    "sort-args=s@" => \$sort_args,
	    "html-args=s@" => \$html_args,
	    "dir-name|dirname|d=s" => \$dir_name,
	    "prefix-exes|P=s" => \$prefix,
	    "help|?"   => \$help,
	    "man"      => \$man);
pod2usage(-exitstatus => 0, -verbose => 1) if $help;
pod2usage(-exitstatus => 0, -verbose => 2) if $man;

if (ref $filter_args)
  { $filter_args = join " ", map { $_ eq "--default" ? $filter_def_args : $_ }
                             @$filter_args; }
if (ref $sort_args)
  { $sort_args   = join " ", map { $_ eq "--default" ? $sort_def_args   : $_ }
                             @$sort_args; }
if (ref $html_args)
  { $html_args   = join " ", map { $_ eq "--default" ? $html_def_args   : $_ }
                             @$html_args; }

pod2usage(0) if (scalar @ARGV != 1);

my $dir_loc   = shift @ARGV;

pod2usage(0) if (! -d $dir_loc);

my $index_loc = $dir_loc . "/index.html";

if (!defined ($dir_name))
  { $dir_name = $dir_loc; }

my $cmds = <<EOL;
 ${prefix}dir_list --size --follow -- '$dir_loc' | \
 ${prefix}dir_filter $filter_args | \
 ${prefix}dir_sort $sort_args | \
 ${prefix}dir_list2html --name '$dir_name' $html_args > '$index_loc'
EOL

system($cmds);

__END__

=head1 NAME

make_index.pl - Make index.html files from directories

=head1 SYNOPSIS

make_index.pl [options] <dir>

 Options:
  --help -?         brief help message
  --man             full documentation
  --filter-args     Filter arguments
  --sort-args       Sort arguments
  --html-args       HTML conversion arguments
  --dir-name        Directory name
  --prefix-exes     prefix for executables

=head1 OPTIONS

=over 8

=item B<--help>

Print a brief help message and exits.

=item B<--man>

Prints the manual page and exits.

=item B<--filter-args>

Args to pass the dir_filter.

=item B<--sort-args>

Args to pass the dir_sort.

=item B<--html-args>

Args to pass the dir_list2html.

=item B<--dir-name>

Name of directory for the title.

=item B<--prefix>

Prefix to add the executable names (can be a path).

=back


=head1 DESCRIPTION

B<make_index.pl> will create index.html files from a given directory.
 It filters the filenames in the directory and can use an executable prefix.

B<make_index.pl> calls the programs dir_list, dir_filter, dir_sort and
dir_list2html.

=cut
