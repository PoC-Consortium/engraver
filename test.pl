#!/usr/bin/env perl

use strict;
use warnings;

my $plotbin = './plot64';

print qx{$plotbin -k 12345 -d core0 -x 0 -s 0 -n 128 -t 4};
print qx{md5sum core0/12345_0_128_128};

print qx{$plotbin -k 12345 -d core1 -x 1 -s 0 -n 128 -t 4};
print qx{md5sum core1/12345_0_128_128};

print qx{$plotbin -k 12345 -d core2 -x 2 -s 0 -n 128 -t 4};
print qx{md5sum core2/12345_0_128_128};
