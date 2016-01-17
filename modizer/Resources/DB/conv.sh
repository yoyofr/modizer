if [ $# -lt 1 ] 
then
  echo "Replace the \t (tabulation) with \\"
  echo "Usage : conv.sh <input>"
  exit 0
fi
# sed 's/	/\\/g' $1 > conv1.tmp
perl -CSAD -pe 's/\t/\//g' $1 > conv1.tmp
rm conv.tmp
for line in `cat ext.txt` 
do
  echo $line
  echo "// {if (tolower(\$NF)==tolower(\"$line\")) print \$0;}" > awk.cmd
  awk -F '.' -f awk.cmd conv1.tmp >> conv.tmp
done
awk -F '/' '// {if (NF==3) printf("%s\\%s/%s\n",$0,$2,$3);}' conv.tmp > comp3.txt
awk -F '/' '// {if (NF==4) printf("%s\\%s/%s/%s\n",$0,$2,$3,$4);}' conv.tmp > comp4.txt
awk -F '/' '// {if (NF==5) printf("%s\\%s/%s/%s/%s\n",$0,$2,$3,$4,$5);}' conv.tmp > comp5.txt
awk -F '/' '// {if (NF==6) printf("%s\\%s/%s/%s/%s/%s\n",$0,$2,$3,$4,$5,$6);}' conv.tmp > comp6.txt
awk -F '/' '// {if (NF==7) printf("%s\\%s/%s/%s/%s/%s/%s\n",$0,$2,$3,$4,$5,$6,$7);}' conv.tmp > comp7.txt
awk -F '/' '// {if (NF==8) printf("%s\\%s/%s/%s/%s/%s/%s/%s\n",$0,$2,$3,$4,$5,$6,$7,$8);}' conv.tmp > comp8.txt
awk -F '/' '// {if ((NF<2)||(NF>8)) printf("%d\\%s\n",NF,$0);}' conv.tmp
# rm conv.tmp
# rm conv1.tmp
