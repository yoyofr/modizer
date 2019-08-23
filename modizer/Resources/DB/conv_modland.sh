perl -CSAD -pe 's/\t/\//g' allmods.txt > conv1.tmp
rm conv.tmp
for line in `cat ext.txt` 
do
  echo $line
  echo "// {if (tolower(\$NF)==tolower(\"$line\")) print \$0;}" > awk.cmd
  awk -F '.' -f awk.cmd conv1.tmp >> conv.tmp
done
awk -F '/' '// {if (NF==2) printf("%s\\%s\\%s\\%s/%s\n",$1,$2,$2);}' conv.tmp > comp2.txt
awk -F '/' '// {if (NF==3) printf("%s\\%s\\%s\\%s/%s\n",$1,$2,$3,$2,$3);}' conv.tmp > comp3.txt
awk -F '/' '// {if (NF==4) printf("%s\\%s\\%s\\%s\\%s/%s/%s\n",$1,$2,$3,$4,$2,$3,$4);}' conv.tmp > comp4.txt
awk -F '/' '// {if (NF==5) printf("%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s\n",$1,$2,$3,$4,$5,$2,$3,$4,$5);}' conv.tmp > comp5.txt
awk -F '/' '// {if (NF==6) printf("%s\\%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s\n",$1,$2,$3,$4,$5,$6,$2,$3,$4,$5,$6);}' conv.tmp > comp6.txt
awk -F '/' '// {if (NF==7) printf("%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s/%s\n",$1,$2,$3,$4,$5,$6,$7,$2,$3,$4,$5,$6,$7);}' conv.tmp > comp7.txt
awk -F '/' '// {if (NF==8) printf("%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s/%s/%s\n",$1,$2,$3,$4,$5,$6,$7,$8,$2,$3,$4,$5,$6,$7,$8);}' conv.tmp > comp8.txt
awk -F '/' '// {if (NF==8) printf("%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s/%s/%s\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$2,$3,$4,$5,$6,$7,$8,$9);}' conv.tmp > comp9.txt
awk -F '/' '// {if ((NF<=1)||(NF>9)) printf("%d\\%s\n",NF,$0);}' conv.tmp > comp_exception.txt
rm conv.tmp
rm conv1.tmp
