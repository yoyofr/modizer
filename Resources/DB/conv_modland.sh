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

# 2. Filtrer par extension avec awk (lookup O(1) via tableau associatif)
# Beaucoup plus rapide que grep avec un pattern de 2000+ alternatives
# Supporte aussi une blacklist (blacklist.txt) pour exclure certaines lignes
awk -F'/' '
BEGIN {
    # Charger toutes les extensions dans un tableau associatif (en minuscules)
    while ((getline ext < "ext.txt") > 0) {
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", ext)  # trim
        if (ext != "") extensions[tolower(ext)] = 1
    }
    close("ext.txt")

    # Charger la blacklist (si le fichier existe)
    blacklist_count = 0
    while ((getline pattern < "blacklist.txt") > 0) {
        gsub(/^[[:space:]]+|[[:space:]]+$/, "", pattern)  # trim
        if (pattern != "" && substr(pattern, 1, 1) != "#") {  # ignorer lignes vides et commentaires
            blacklist_count++
            blacklist[blacklist_count] = pattern
        }
    }
    close("blacklist.txt")
}

function is_blacklisted(line) {
    for (i = 1; i <= blacklist_count; i++) {
        if (index(line, blacklist[i]) > 0) {
            return 1
        }
    }
    return 0
}

{
    # Verifier la blacklist en premier
    if (is_blacklisted($0)) next

    # Extraire l extension du dernier champ (nom de fichier)
    filename = $NF
    # Trouver la dernière occurrence de "."
    n = split(filename, parts, ".")
    if (n > 1) {
        ext = tolower(parts[n])
        if (ext in extensions) {
            print
            next
        }
    }
    # Vérifier aussi si le nom de fichier commence par une extension connue (format "EXT.xxx")
    if (n >= 1) {
        ext = tolower(parts[1])
        if (ext in extensions) {
            print
        }
    }
}' conv1.tmp | sort -u > conv.tmp
cp conv.tmp convgrep.yo
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
