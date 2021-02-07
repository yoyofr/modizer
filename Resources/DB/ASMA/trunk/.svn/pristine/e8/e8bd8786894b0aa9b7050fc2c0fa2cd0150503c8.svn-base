#!/usr/bin/perl
use strict;

my $n;

sub process_dir($) {
	my ($d) = @_;
	opendir my $h, $d or die "$d: $!\n";
	my @e = sort grep !/^\./, readdir $h;
	closedir $h;
	my %h;
	if (@e > 64) {
		printf "%s: %d entries\n", $d, scalar @e;
	}
	for my $e (@e) {
		my $l = "$d/$e";
		my $s = uc $e;
		$s =~ y/-_//d;
		my ($f, $x) = $s =~ /^([0-9A-Z]+)(\.[0-9A-Z]{1,3})?$/ or die "$l: illegal filename\n";
		$f = substr($f, 0, $n);
		my $c = 1;
		while ($h{$f.$x}++) {
			++$c;
			$f = substr($f, 0, $n - length($c)) . $c;
		}
		printf "%-*s%-5s%s\n", $n, $f, $x, $l;
		if (-d $l) {
			process_dir($l);
		}
	}
}

if (@ARGV != 2 || $ARGV[0] !~ /^\d\d?$/) {
	die "Usage: perl shorten-filenames.pl NUMCHARS DIR\n";
}
$n = $ARGV[0];
process_dir($ARGV[1]);
