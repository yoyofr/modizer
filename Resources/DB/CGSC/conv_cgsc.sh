awk -F'=' -f cgsc_pathmd5.awk $1 > cgsc_pathmd5.txt

awk -F '/' '// {if (NF==2) printf("%s\n",$0);}' cgsc_pathmd5.txt > cgsc2.txt
awk -F '/' '// {if (NF==3) printf("%s\n",$0);}' cgsc_pathmd5.txt > cgsc3.txt
awk -F '/' '// {if (NF==4) printf("%s\n",$0);}' cgsc_pathmd5.txt > cgsc4.txt
awk -F '/' '// {if (NF==5) printf("%s\n",$0);}' cgsc_pathmd5.txt > cgsc5.txt
awk -F '/' '// {if (NF==6) printf("%s\n",$0);}' cgsc_pathmd5.txt > cgsc6.txt
awk -F '/' '// {if (NF==7) printf("%s\n",$0);}' cgsc_pathmd5.txt > cgsc7.txt
awk -F '/' '// {if (NF==8) printf("%s\n",$0);}' cgsc_pathmd5.txt > cgsc8.txt
