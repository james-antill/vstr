#! /usr/bin/perl -w

use strict;

use File::Temp qw/tempfile/;

use FileHandle;

my $prefix = "j";

if (-f "./ex_dir_list")
  { $prefix = "./ex_"; }

my $dir_name   = '"' . $ARGV[0] . '"';

my $filter_args = "-A .. -D index.html -D dir_list.css --deny-name-end .gz --deny-name-end .tmp --deny-name-end '~' --deny-name-end '#'";

if (0)
  { $filter_args .= " --deny-name-beg ."; }

my $cmds = <<EOL;
 ${prefix}dir_list --size --follow -- $dir_name | \
 ${prefix}dir_filter $filter_args | \
 ${prefix}dir_sort --without-locale | \
 ${prefix}dir_list2html --name $dir_name > $dir_name/index.html
EOL

system($cmds);
