#!/bin/bash
# MODLAND
echo downloading last allmods.zip
echo ...
rm allmods.zip
curl https://ftp.modland.com/allmods.zip --output allmods.zip
echo unzipping
echo ...
unzip allmods.zip
echo removing HVSC
grep -v "\tHVSC/" allmods.txt > tmp.txt
mv tmp.txt allmods.txt
echo converting
echo ...
./conv_modland.sh allmods.txt
# HVSC
echo HVSC
echo ...
cd HVSC
./conv_stil.sh
./conv_hvscSL.sh
cd ..
# ASMA
echo ASMA
echo ...
cd ASMA
./get_asma_latest.sh
./gen_new_asma.sh Asma
cp trunk/asma/Docs/STIL.txt .
./conv_stil.sh
./conv_asma.sh Asma.txt
cd ..
# SID CGSC
echo CGSC
echo ...
cd CGSC
./gen_new_cgsc.sh Cgsc
./conv_cgsc.sh Cgsc.txt
cd ..
# Create Main DB
echo Create DB
./create_db.sh
rm comp*.txt
rm allmods.txt
rm allmods.zip
rm ASMA/asma*.txt
rm CGSC/cgsc*.txt
rm HVSC/hvsc*.txt

