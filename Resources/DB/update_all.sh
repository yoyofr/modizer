#!/bin/bash

echo allmods
echo ...
unzip allmods.zip
./conv_modland.sh allmods.txt
echo HVSC
echo ...
cd HVSC
./conv_stil.sh
./conv_hvscSL.sh
cd ..
echo ASMA
echo ...
cd ASMA
./get_asma_latest.sh
./gen_new_asma.sh Asma
cp trunk/Extras/Docs/STIL.txt .
./conv_stil.sh
./conv_asma.sh Asma.txt

cd ..
echo Create DB
./create_db.sh
rm comp*.txt
rm allmods.txt
rm ASMA/asma*.txt
rm HVSC/hvsc*.txt
