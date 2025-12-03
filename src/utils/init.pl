#!/usr/bin/perl -w
#  init.pl
#
###
###  function
###
#
#  Copyright (Perl) Jie
#  2025-12-03
# 
use 5.38.0;
use utf8::all;
use lib '/home/jie/scripts/perl/';

my @files = glob "*.c";
my $content = `cat touch.c`;

for (@files) {
    system "cat touch.c > $_";
}
