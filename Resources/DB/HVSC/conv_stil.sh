rm STILconv.txt
iconv -f LATIN1 -t UTF-8 STIL.txt > STILconv.txt
awk -f stil.awk STILconv.txt > stilconverted
rm STILconv.txt
