#!/bin/bash

if (( $# < 1))
then
  printf "%b" "allmods file missing.\n" >&2
  printf "%b" "usage: conv_modland.sh allmod_text_file\n" >&2
  printf "\n" >&2
  exit 1
fi

# 1. Convertir tabs en /
perl -CSAD -pe 's/\t/\//g' allmods.txt > conv1.tmp

# 2. Construire un pattern grep unique avec toutes les extensions (échapper les caractères spéciaux regex)
EXT_PATTERN=$(sed 's/[.[\*^$()+?{|]/\\&/g; s/!/\\!/g' ext.txt | paste -sd'|' -)

# 3. Un seul grep case-insensitive avec toutes les extensions, puis sort | uniq
grep -iE "$EXT_PATTERN" conv1.tmp | sort -u > conv.tmp

# 4. Un seul awk pour générer tous les fichiers de sortie
awk -F '/' '
BEGIN {
  # Vider les fichiers de sortie
  print "" > "comp2.txt"; close("comp2.txt")
  print "" > "comp3.txt"; close("comp3.txt")
  print "" > "comp4.txt"; close("comp4.txt")
  print "" > "comp5.txt"; close("comp5.txt")
  print "" > "comp6.txt"; close("comp6.txt")
  print "" > "comp7.txt"; close("comp7.txt")
  print "" > "comp8.txt"; close("comp8.txt")
  print "" > "comp9.txt"; close("comp9.txt")
  print "" > "comp_exception.txt"; close("comp_exception.txt")
}
{
  nf = NF
  if (nf == 2) {
    printf("%s\\%s\\%s\n", $1, $2, $2) >> "comp2.txt"
  }
  else if (nf == 3) {
    printf("%s\\%s\\%s\\%s/%s\n", $1, $2, $3, $2, $3) >> "comp3.txt"
  }
  else if (nf == 4) {
    if (($2 == "Ad Lib")||($2 == "Spectrum")||($2 == "Video Game Music")) {
      printf("%s\\%s %s\\%s\\%s/%s/%s\n", $1, $2, $3, $4, $2, $3, $4) >> "comp3.txt"
    } else {
      printf("%s\\%s\\%s\\%s\\%s/%s/%s\n", $1, $2, $3, $4, $2, $3, $4) >> "comp4.txt"
    }
  }
  else if (nf == 5) {
    if (($2 == "Ad Lib")||($2 == "Spectrum")||($2 == "Video Game Music")) {
      printf("%s\\%s %s\\%s\\%s\\%s/%s/%s/%s\n", $1, $2, $3, $4, $5, $2, $3, $4, $5) >> "comp4.txt"
    } else {
      printf("%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s\n", $1, $2, $3, $4, $5, $2, $3, $4, $5) >> "comp5.txt"
    }
  }
  else if (nf == 6) {
    if (($2 == "Ad Lib")||($2 == "Spectrum")||($2 == "Video Game Music")) {
      printf("%s\\%s %s\\%s\\%s\\%s\\%s/%s/%s/%s/%s\n", $1, $2, $3, $4, $5, $6, $2, $3, $4, $5, $6) >> "comp5.txt"
    } else {
      printf("%s\\%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s\n", $1, $2, $3, $4, $5, $6, $2, $3, $4, $5, $6) >> "comp6.txt"
    }
  }
  else if (nf == 7) {
  if (($2 == "Ad Lib")||($2 == "Spectrum")||($2 == "Video Game Music")) {
      printf("%s\\%s %s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s/%s\n", $1, $2, $3, $4, $5, $6, $7, $2, $3, $4, $5, $6, $7) >> "comp6.txt"
    } else {
      printf("%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s/%s\n", $1, $2, $3, $4, $5, $6, $7, $2, $3, $4, $5, $6, $7) >> "comp7.txt"
    }
  }
  else if (nf == 8) {
    printf("%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s/%s/%s\n", $1, $2, $3, $4, $5, $6, $7, $8, $2, $3, $4, $5, $6, $7, $8) >> "comp8.txt"
  }
  else if (nf == 9) {
    printf("%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s\\%s/%s/%s/%s/%s/%s/%s\n", $1, $2, $3, $4, $5, $6, $7, $8, $9, $2, $3, $4, $5, $6, $7, $8, $9) >> "comp9.txt"
  }
  else {
    printf("%d\\%s\n", nf, $0) >> "comp_exception.txt"
  }
}' conv.tmp

# Supprimer les lignes vides en début de fichier (créées par le BEGIN)
for f in comp2.txt comp3.txt comp4.txt comp5.txt comp6.txt comp7.txt comp8.txt comp9.txt comp_exception.txt; do
  if [ -f "$f" ]; then
    sed -i '' '1{/^$/d;}' "$f"
  fi
done

# Nettoyage
rm -f conv.tmp conv1.tmp
