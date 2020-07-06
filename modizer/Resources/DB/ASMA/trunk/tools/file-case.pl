my %f;
while (<>) {
	s/^.+\\asma\\// or die $_;
	y!\\\r\n!/!d;
	$f{$_}++;
}
open F, '../asma/new.m3u' or die;
while (<F>) {
	y/\r\n//d;
	$f{$_} or print "new.m3u: $_\n";
}
open F, '../asma/Docs/STIL.txt' or die;
while (<F>) {
	y/\r\n//d;
	$f{$_} or print "STIL.txt: $_\n" if s!^/!!;
}
