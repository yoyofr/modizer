#!/usr/bin/perl
#
# iodump-decoder.pl - decode plugout_iodumper output to plain text
#
# 2018 (C) by Christian Garbs <mitch@cgarbs.de>
# Licensed under GNU GPL v1 or, at your option, any later version
#
# basic usage:
#
# $ ./gbsplay -o iodumper examples/nightmode.gbs | contrib/iodump-decoder.pl
#
# to find certain events:
#
# $ ./gbsplay -o iodumper examples/nightmode.gbs \
#     | contrib/iodump-decoder.pl \
#     | grep ch3 \
#     | grep TRIG.1.
#

use strict;
use warnings;

my $remove_unused_bits = 1;

sub decode
{
    my ($template, $val) = @_;

    # expand bit ranges
    $template =~ s/\$(\d)\.\.\$(\d)/join '', map { "\$$_" } reverse ($2 .. $1)/eg;

    # replace bits
    for my $bit (0..7) {
	my $v = ($val & (1 << $bit)) ? 1 : 0;
	$template =~ s/\$$bit/$v/g;
    }

    $template =~ s/ _\[[01]+\]//g if $remove_unused_bits;
    
    return $template;
}

my $cycle;

while (my $line = <>) {

    chomp $line;

    next if ($line eq '');

    if ($line =~ 'subsong (\d+)') {
	# subsong switch
	$cycle = 0;
	print "$line\n";
	next;
    }
    
    die "unparseable line: `$line'" unless $line =~ /^([0-9a-f]{8,16}) ([0-9a-f]{4})=([0-9a-f]{2})/;
    
    my ($cycle_diff, $addr, $val) = ($1, $2, hex $3);

    # on subsong change/reset, iodumper will print negative(?) cycle diff - just ignore it
    $cycle_diff = '0x0' if length $cycle_diff == 16;

    $cycle += hex $cycle_diff;

    my $ms = $cycle * 1000 / 4194304;

    my $ss = int($ms / 1000);
    $ms %= 1000;

    my $mm = int($ss / 60);
    $ss %= 60;

    my $tsp = sprintf '%02d:%02d.%04d', $mm, $ss, $ms;

    my $msg = '';
    
    if ($addr eq 'ff10') {
	$msg = decode 'ch1 _[$7] SPER[$6..$4] SNEG[$3] SSHI[$2..$0]', $val;
    }
    elsif ($addr eq 'ff11') {
	$msg = decode 'ch1 DUTY[$7..$6] LEN[$5..$0]', $val;
    }
    elsif ($addr eq 'ff12') {
	$msg = decode 'ch1 VOL[$7..$4] EADD[$3] EPER[$2..$0]', $val;
    }
    elsif ($addr eq 'ff13') {
	$msg = decode 'ch1 FLSB[$7..$0]', $val;
    }
    elsif ($addr eq 'ff14') {
	$msg = decode 'ch1 TRIG[$7] LENBL[$6] _[$5..$3] FMSB[$2..$0]', $val;
    }

# unused register
#   elsif ($addr eq 'ff15') {
#	$msg = decode 'ch2 _[$7..$0]', $val;
#   }
    elsif ($addr eq 'ff16') {
	$msg = decode 'ch2 DUTY[$7..$6] LEN[$5..$0]', $val;
    }
    elsif ($addr eq 'ff17') {
	$msg = decode 'ch2 VOL[$7..$4] EADD[$3] EPER[$2..$0]', $val;
    }
    elsif ($addr eq 'ff18') {
	$msg = decode 'ch2 FLSB[$7..$0]', $val;
    }
    elsif ($addr eq 'ff19') {
	$msg = decode 'ch2 TRIG[$7] LENBL[$6] _[$5..$3] FMSB[$2..$0]', $val;
    }
    
    elsif ($addr eq 'ff1a') {
	$msg = decode 'ch3 DAC[$7] _[$6..$0]', $val;
    }
    elsif ($addr eq 'ff1b') {
	$msg = decode 'ch3 LEN[$7..$0]', $val;
    }
    elsif ($addr eq 'ff1c') {
	$msg = decode 'ch3 _[$7] VOL[$6$5] _[$4..$0]', $val;
    }
    elsif ($addr eq 'ff1d') {
	$msg = decode 'ch3 FLSB[$7..$0]', $val;
    }
    elsif ($addr eq 'ff1e') {
	$msg = decode 'ch3 TRIG[$7] LENBL[$6] _[$5..$3] FMSB[$2..$0]', $val;
    }

# unused register
#   elsif ($addr eq 'ff1f') {
#	$msg = decode 'ch4 _[$7..$0]', $val;
#   }
    elsif ($addr eq 'ff20') {
	$msg = decode 'ch4 _[$7..$6] LEN[$5..$0]', $val;
    }
    elsif ($addr eq 'ff21') {
	$msg = decode 'ch4 VOL[$7..$4] EADD[$3] EPER[$2..$0]', $val;
    }
    elsif ($addr eq 'ff22') {
	$msg = decode 'ch4 CLSH[$7..$4] WIDT[$3] DIV[$2..$0]', $val;
    }
    elsif ($addr eq 'ff23') {
	$msg = decode 'ch4 TRIG[$7] LENBL[$6] _[$5..$0]', $val;
    }

    elsif ($addr eq 'ff24') {
	$msg = decode 'LVIN[$7] LVOL[$6..$4] RVIN[$3] RVOL[$2..$0]', $val;
    }
    elsif ($addr eq 'ff25') {
#	$msg = decode 'LCH4[$7] LCH3[$6] LCH2[$5] LCH1[$4] RCH4[$3] RCH3[$2] RCH2[$1] RCH1[$0]', $val;
	$msg = decode 'enables L/R ch1[$4$0] ch2[$5$1] ch3[$6$2] ch4[$7$3]', $val;
    }
    elsif ($addr eq 'ff26') {
	$msg = decode 'POWR[$7] _[$6..$4] SCH4[$3] SCH3[$2] SCH2[$1] SCH1[$0]', $val;
    }
    
    
    printf "%s %08x %s %s\n", $tsp, $cycle, $line, $msg;
}
