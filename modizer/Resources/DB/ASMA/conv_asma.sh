awk -F'=' -f asma_pathmd5.awk $1 > asma_pathmd5.txt

awk -F '/' '// {if (NF==2) printf("%s\n",$0);}' asma_pathmd5.txt > asma2.txt
awk -F '/' '// {if (NF==3) printf("%s\n",$0);}' asma_pathmd5.txt > asma3.txt
awk -F '/' '// {if (NF==4) printf("%s\n",$0);}' asma_pathmd5.txt > asma4.txt
awk -F '/' '// {if (NF==5) printf("%s\n",$0);}' asma_pathmd5.txt > asma5.txt
awk -F '/' '// {if (NF==6) printf("%s\n",$0);}' asma_pathmd5.txt > asma6.txt
awk -F '/' '// {if (NF==7) printf("%s\n",$0);}' asma_pathmd5.txt > asma7.txt
awk -F '/' '// {if (NF==8) printf("%s\n",$0);}' asma_pathmd5.txt > asma8.txt
