#!/usr/bin/env perl
# For Emacs: -*- mode:cperl -*-


use strict;
use warnings;

use Carp;
use Getopt::Long;                                                # command line options processing

my $plotbin  = './plot64';
my $md5sum   = ($^O eq "darwin") ? 'md5 -q' : 'md5sum';
my $expected = '4f81804ea010744877163a87f56fc225';
my $keep = 0;

GetOptions(
    'keep'  => \$keep,             # keep generated test files
) or croak "Formal error processing command line options!";


if (! -x $plotbin) {
    print "$plotbin binary not present. Compile it first.\n";
    exit 1;
}


# Test Core 0 (scalar)
print qx{$plotbin -a -v -k 11424087411148401423 -d core0 -x 0 -s 0 -n 128 -t 4};
cmp_digest('core0/11424087411148401423_0_128', $expected);

# Test Core 1 (SSE4)
print qx{$plotbin -a -v -k 11424087411148401423 -d core1 -x 1 -s 0 -n 128 -t 4};
cmp_digest('core1/11424087411148401423_0_128', $expected);

# Test Core 2 (AVX2) if not on TRAVIS
if (defined $ENV{TRAVIS} && $ENV{TRAVIS} =~ m{true}xmsi) {
    print "Skipping AVX2 test as TRAVIS does not support it.\n"
}
else {
    print qx{$plotbin -a -v -k 11424087411148401423 -d core2 -x 2 -s 0 -n 128 -t 4};

    cmp_digest('core2/11424087411148401423_0_128', $expected);
}

# cleanup
qx{rm -rf core0 core1 core2} if (!$keep);

sub cmp_digest {
    my $file   = shift;
    my $expect = shift;

    my $digest = `$md5sum $file`;

    chomp $digest;

    if ($digest =~ m{^\Q$expected}) {
        print "Digest OK\n"
    }
    else {
        print "Digest did not match.  Expected $expected got $digest\n";
        exit 1;
    }

    return;
}
