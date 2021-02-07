perl -p0777i -e "s/^([^\xFF]+.{5067})L(.{431})L(.+)\xe0\x02\xe1\x02..$/$1\x60$2\x60$3/s||print STDERR qq{$ARGV\n}" ../asma/Unknown/Rock_1.sap
