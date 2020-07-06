grep -E " $" ../asma/Docs/STIL.txt
grep -E ".{100}" ../asma/Docs/STIL.txt
grep -v -n -E "^#|^/|^   NAME: |^  TITLE: |^ ARTIST: |^COMMENT: |^         |^$|\(#[0-9]+\)$" ../asma/Docs/STIL.txt
perl -ne "m{^/(\S+)} and (-e qq{../asma/$1} or print)" ../asma/Docs/STIL.txt
@rem grep "^/" ../asma/Docs/STIL.txt | sort -cfg
grep -c "^/" ../asma/Docs/STIL.txt
