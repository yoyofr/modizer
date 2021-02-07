open F, '../asma/Docs/Composers.txt' or die;
1 until <F> =~ /^Unknown - /;
<F>;
my $n;
my %countries;
my ($real, $nick, $country) = '';
while (<F>) {
	print if $_ le $real;
	if (($real, $nick, $country) = /^(.+?\S, .+?\S|<\?>) - (?:(.+?\S) - )?(.*\S)/) {
		$n++;
		$countries{$country}++;
	}
	else {
		print;
	}
}
print "Composers: $n\n";
print "$_ - $countries{$_}\n" for sort keys %countries;
