tail +2 Songlengths.md5 > tmp
awk -F'[\=\ ]' -f sid_songlength.awk tmp > hvsc_songl.txt
awk -F'[\=\ ]' -f sid_pathmd5.awk tmp > hvsc_pathmd5.txt

awk -F '/' '// {if (NF==3) printf("%s\n",$0);}' hvsc_pathmd5.txt > hvsc3.txt
awk -F '/' '// {if (NF==4) printf("%s\n",$0);}' hvsc_pathmd5.txt > hvsc4.txt
awk -F '/' '// {if (NF==5) printf("%s\n",$0);}' hvsc_pathmd5.txt > hvsc5.txt
awk -F '/' '// {if (NF==6) printf("%s\n",$0);}' hvsc_pathmd5.txt > hvsc6.txt
awk -F '/' '// {if (NF==7) printf("%s\n",$0);}' hvsc_pathmd5.txt > hvsc7.txt
awk -F '/' '// {if (NF==8) printf("%s\n",$0);}' hvsc_pathmd5.txt > hvsc8.txt


rm tmp
