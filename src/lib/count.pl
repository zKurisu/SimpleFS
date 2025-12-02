#!/usr/bin/perl -w
#  count.pl
#
###
###  function
###
#
#  Copyright (Perl) Jie
#  2025-11-19
# 
use 5.38.0;
use utf8::all;
use lib '/home/jie/scripts/perl/';

my @files = glob "*";
my $count = 0;

for (@files) {
    my $output = `wc $_`;
    my @info = split " ", $output;
    $count += $info[0];
}

print $count;
