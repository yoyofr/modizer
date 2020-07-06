my $r = shift or die "Usage: perl svn2new.pl STARTING_SVN_REVISION\n";
my @i = `svn log -qv -r $r:HEAD ../asma`;
my (%a, %d);
my @d;
for (@i) {
	if (m{^   A /trunk/asma/(\S+\.sap)(?: \(from /trunk/asma/(\S+\.sap):\d+\))?}) {
		$a{$1} = 1, delete $d{$1} if !$2 || delete $a{$2};
	}
	elsif (m{^   D /trunk/asma/(\S+)}) {
		# postpone deletes after adds, so that copy+delete in one commit works
		push @d, $1;
	}
	elsif (m{^---}) {
		for (@d) {
			$d{$_} = 1;
			if (/\.sap$/) {
				delete $a{$_};
			}
			else {
				my $dir = $_;
				delete @a{grep m{^$dir/}, keys %a};
			}
		}
		@d = ();
	}
}
open F, ">new.txt" or die;
print F "$_\n" for sort keys %a;
open F, ">deleted.txt" or die;
print F "$_\n" for sort keys %d;
printf "New: %d Deleted: %d\n", scalar(keys %a), scalar(keys %d);
