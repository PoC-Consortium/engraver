#!/usr/bin/env perl
use strict;
use warnings;

my $plotbin = './plot64';
my $md5sum = 'md5sum';
if ($^O eq "darwin") {
	$md5sum = 'md5 -q';
}

my $expected =  "3ba11bee42182e0684df470d420c813e";

print qx{$plotbin -a -v -k 11424087411148401423 -d core0 -x 0 -s 0 -n 128 -t 4};
my $digest = `$md5sum core0/11424087411148401423_0_128_128`;
chomp($digest);
($digest) = $digest =~ /^(\w+)/;
if ($digest eq $expected) {
	print "Digest OK\n"
} else {
	print "Digest did not match.  Expected $expected got $digest\n";
	exit 1;
}

print qx{$plotbin -a -v -k 11424087411148401423 -d core1 -x 1 -s 0 -n 128 -t 4};
$digest = `$md5sum core1/11424087411148401423_0_128_128`;
chomp($digest);
($digest )= $digest =~ /^(\w+)/;
if ($digest eq $expected) {
	print "Digest OK\n"
} else {
	print "Digest did not match.  Expected $expected got $digest\n";
	exit 1;
}


print qx{$plotbin -a -v -k 11424087411148401423 -d core2 -x 2 -s 0 -n 128 -t 4};
$digest = `$md5sum core2/11424087411148401423_0_128_128`;
chomp($digest);
($digest) = $digest =~ /^(\w+)/;
if ($digest eq $expected) {
	print "Digest OK\n"
} else {
	print "Digest did not match.  Expected $expected got $digest\n";
	exit 1;
}

