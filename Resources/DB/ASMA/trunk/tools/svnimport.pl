# TODO: svn mv --parents Various/Swierszcz_Piotr/Energy_1_2.sap Various/Swierszcz_Piotr/Last_World.sap

use File::Basename;
use File::Find;
use POSIX qw(strftime);

#my $repo = 'file:///C:/0/a8/asap/aasma/import/repo';
my $repo = 'svn://asma.scene.pl/asma';
my $globopt = '--username pfusik';

my @dirs;

sub wanted() {
	push @dirs, $File::Find::name if -d;
}

sub dirs($) {
	my ($dir) = @_;
	@dirs = ();
	find(\&wanted, $dir);
	shift @dirs;
	return map { s!^\Q$dir\E/!!; $_; } @dirs;
}

sub indir($\@) {
	my ($file, $dirs) = @_;
	my @r = grep $file =~ m!^$_/!, @$dirs;
	return pop @r;
}

sub shell($) {
	print @_, "\n";
#	system @_;
}

sub commit($$$$) {
	my ($d, $log, $tag, $taglog) = @_;
	shell "svn commit $globopt -m '$log' ..";
	my $f = (-e "$d/Docs/Asma.txt") ? "$d/Docs/Asma.txt" : "$d/Docs/Readme.txt";
	my $t = POSIX::strftime('%FT%T.000000Z', localtime((stat $f)[9]));
	shell "svn propset $globopt svn:date --revprop -r HEAD $t";
	shell "svn cp $globopt -m '$taglog' $repo/trunk/asma $repo/tags/$tag";
	shell "svn propset $globopt svn:date --revprop -r HEAD $t";
}

if (0) {
	shell "rm -rf trunk repo";
	shell "svnadmin create --fs-type fsfs repo";
	shell "touch repo/hooks/pre-revprop-change.bat";
	shell "svn mkdir -m 'Repository init' $repo/trunk $repo/tags $repo/branches";
	shell "svn co $repo/trunk trunk";
}
shell 'set -e';
shell 'cd trunk';
shell 'mkdir asma && svn add asma && cd asma';
shell "cp -r ../../ASMA05/* .";
shell "svn add *";
commit('ASMA05', 'Imported ASMA 0.5', 'asma-0.5', 'PRE-RELEASE ASMA 0.5');
my $p = 'ASMA05';
for my $d ('ASMA06' .. 'ASMA35') {
	shell "echo -------- $d --------";
	my @pd = dirs($p);
	my @cd = dirs($d);
	my %pd = map { $_ => 1 } @pd;
	my %cd = map { $_ => 1 } @cd;
	my @rd = grep !$cd{$_}, @pd;
	my @ad = grep !$pd{$_}, @cd;
	my %md;
	my %nd;
	my @g = `git diff -M -l99999 --name-status $p $d`;
	my @a;
	if ($d eq 'ASMA29') {
		shell "svn mv Docs/Update1$_.txt Docs/Update2$_.txt" for 0 .. 8;
		shell "svn mv Docs/Update0$_.txt Docs/Update1$_.txt" for 1 .. 9;
	}
	elsif ($d eq 'ASMA33') {
		shell "svn mv Various Composers";
		shell "svn mv Composers/Benoth_Sukkor Composers/Stanik_Krzysztof";
		$md{'Composers/Benoth_Sukkor'} = 'Composers/Stanik_Krzysztof';
		shell "svn mv Composers/Trokowicz_B Composers/Trokowicz_Bartolomiej";
		$md{'Composers/Trokowicz_B'} = 'Composers/Trokowicz_Bartolomiej';
		shell "svn mv Composers/Zur-soft Composers/Zur_soft";
		$md{'Various/Zur-soft'} = 'Composers/Zur_soft';
	}
	for (@g) {
		if ($d eq 'ASMA29' && m!^(A\t$d|D\t$p)/Docs/Update\d\d\.txt$!) {
		}
		if (m!^A\t$d/(.+)$!) {
			push @a, $1 if $1 ne 'Various/Ramzes/Endless_Dream_5.sap';
		}
		elsif (m!^R0*(\d+)\t$p/(.+?)\t$d/(.+)$!) {
			my ($perc, $rf, $af) = ($1, $2, $3);
			my $rd = indir($rf, @rd);
			my $ad = indir($af, @ad);
			if (defined($rd) && defined($ad)
				&& $ad ne 'Various/Grayscale' && $ad ne 'Composers/Kuczek_Ireneusz/Worktunes'
				&& $rd ne 'Unsorted' && $rd ne 'Demos' && $rd ne 'Games/Various') {
				if (exists($md{$rd})) {
					die $_ if $md{$rd} ne $ad;
				}
				else {
					$md{$rd} = $ad;
					shell "svn mv --parents $rd $ad" unless "$rd $ad" =~ m!^Various/(.+?) Composers/\1$!;
				}
				$rf =~ s/^$rd/$ad/;
				shell "svn mv $rf $af # RENAME $perc%" if $rf ne $af;
			}
			else {
				if ($af eq 'Demos/Ray_of_Hope_2.sap') {
					push @a, $af;
					$af = 'Various/Ramzes/Endless_Dream_5.sap' ;
				}
				if ($af eq 'Various/Szpilowski_Michal/Sej-mik.sap') {
					push @a, $af;
					shell "svn rm $rf";
					next;
				}
				# --parents does this
				#$ad = dirname($af);
				#unless ($nd{$ad} || -d "$p/$ad") {
				#	shell "mkdir -p $ad && svn add $ad";
				#	$nd{$ad} = 1;
				#}
				$rf =~ s/^.*?'.*$/"$&"/;
				shell "svn mv --parents $rf $af # $perc%";
			}
		}
		elsif (m!^D\t$p/(.+)$!) {
			my $f = $1;
			shell "svn rm $f" unless defined indir($f, @rd);
		}
		elsif (m!^M!) {
		}
		else {
			die $_;
		}
	}
	shell "cp -r ../../$d/* .";
	exists($md{$_}) || shell "svn rm $_" for @rd;
	s/^.*?'.*$/"$&"/, shell "svn add --parents $_" for @a;
	$d =~ /(\d)(\d)/;
	my $r = "$1.$2";
	my $pre = $1 ? '' : 'PRE-';
	commit($d, "Imported ASMA $r", "asma-$r", "${pre}RELEASE ASMA $r");
	$p = $d;
	shell "svn up";
}
