# Guide de Captures d'Écran pour Modizer

Ce document explique comment créer et nommer les captures d'écran pour le guide utilisateur.

## 📸 Convention de Nommage

Utilisez le format : `[section]-[description]-[variation].png`

### Structure des Noms de Fichiers

```
[section]:     player | browser | playlist | online | search | downloads | settings | more
[description]: descriptif court en anglais, mots séparés par tirets
[variation]:   portrait | landscape | dark | light (optionnel)
```

## 📋 Liste Complète des Screenshots Nécessaires

### 🎵 PLAYER SCREEN (Écran Principal)
**Priorité : HAUTE**

```
images/player-main-view-portrait.png
images/player-main-view-landscape.png
images/player-playback-controls.png
images/player-track-info.png
images/player-loop-modes.png
images/player-shuffle-modes.png
images/player-rating.png
images/player-subsong-selector.png
images/player-archive-selector.png
images/player-info-panel.png
images/player-visualizer-button.png
images/player-mini-player.png
```

### 🎨 VISUALIZERS
**Priorité : HAUTE**

```
images/viz-projectm-fullscreen.png
images/viz-projectm-preset1.png
images/viz-projectm-preset2.png
images/viz-3d-spectrum.png
images/viz-3d-landscape.png
images/viz-piano-3d.png
images/viz-2d-spectrum.png
images/viz-piano-roll.png
images/viz-mod-pattern.png
images/viz-oscilloscope.png
images/viz-preset-selector.png
```

### 📁 FILE BROWSER
**Priorité : HAUTE**

```
images/browser-main-view.png
images/browser-folder-navigation.png
images/browser-search-bar.png
images/browser-file-list.png
images/browser-context-menu.png
images/browser-file-details.png
images/browser-create-folder.png
images/browser-rename-file.png
images/browser-archive-contents.png
images/browser-icloud-folder.png
```

### 📋 PLAYLISTS
**Priorité : HAUTE**

```
images/playlist-main-view.png
images/playlist-smart-playlists.png
images/playlist-now-playing.png
images/playlist-favorites.png
images/playlist-most-played.png
images/playlist-create-new.png
images/playlist-add-song.png
images/playlist-edit-mode.png
images/playlist-reorder.png
images/playlist-search.png
```

### 🌐 ONLINE COLLECTIONS
**Priorité : MOYENNE**

```
images/online-main-list.png
images/online-modland-browse.png
images/online-modland-by-author.png
images/online-modland-by-format.png
images/online-hvsc-browse.png
images/online-asma-browse.png
images/online-cgsc-browse.png
images/online-file-download.png
images/online-download-queue.png
images/online-search-results.png
```

### 🔍 SEARCH
**Priorité : MOYENNE**

```
images/search-main-view.png
images/search-typing.png
images/search-results-grouped.png
images/search-local-results.png
images/search-online-results.png
images/search-tap-result.png
```

### 📥 DOWNLOADS
**Priorité : MOYENNE**

```
images/downloads-main-view.png
images/downloads-active-download.png
images/downloads-queue.png
images/downloads-progress-bar.png
images/downloads-completed.png
images/downloads-failed.png
```

### ⚙️ SETTINGS - GENERAL
**Priorité : MOYENNE**

```
images/settings-main-menu.png
images/settings-global.png
images/settings-playback-frequency.png
images/settings-audio-latency.png
images/settings-player-priority.png
images/settings-format-selector.png
```

### 🎛️ SETTINGS - PLUGINS
**Priorité : BASSE**

```
images/settings-plugins-list.png
images/settings-sid.png
images/settings-nsfplay.png
images/settings-openmpt.png
images/settings-adplug.png
images/settings-uade.png
images/settings-timidity.png
```

### 🎨 SETTINGS - VISUALIZER
**Priorité : MOYENNE**

```
images/settings-visualizer-main.png
images/settings-projectm.png
images/settings-oscilloscope.png
images/settings-piano-roll.png
images/settings-mod-pattern.png
images/settings-3d-spectrum.png
```

### 🌐 SETTINGS - ONLINE & FTP
**Priorité : BASSE**

```
images/settings-ftp-server.png
images/settings-ftp-enabled.png
images/settings-online-urls.png
images/settings-modland-config.png
```

### 🎚️ EQUALIZER
**Priorité : MOYENNE**

```
images/eq-main-view.png
images/eq-bands-adjusted.png
images/eq-global-gain.png
images/eq-on-off-toggle.png
```

### 🔊 VOICE/CHANNEL SELECTOR
**Priorité : MOYENNE**

```
images/voices-main-view.png
images/voices-all-channels.png
images/voices-mute-channel.png
images/voices-solo-mode.png
images/voices-multi-chip.png
```

### 📱 MORE MENU
**Priorité : BASSE**

```
images/more-main-menu.png
images/more-about.png
images/more-tips.png
images/more-voice-commands.png
images/more-maintenance.png
```

### 🌍 WEB BROWSER
**Priorité : BASSE**

```
images/web-browser-main.png
images/web-bookmarks.png
images/web-navigation.png
```

### 🚗 ADDITIONAL FEATURES
**Priorité : BASSE**

```
images/feature-carplay.png
images/feature-notifications.png
images/feature-lock-screen.png
images/feature-icloud.png
images/feature-ftp-connection.png
```

---

## 📐 Spécifications Techniques

### Résolutions Recommandées

**iPhone (Portrait) :**
- iPhone 15 Pro Max : 1290 x 2796 px
- iPhone 14 Pro : 1179 x 2556 px
- iPhone SE : 750 x 1334 px

**iPhone (Landscape) :**
- iPhone 15 Pro Max : 2796 x 1290 px
- iPhone 14 Pro : 2556 x 1179 px

**iPad (Portrait) :**
- iPad Pro 12.9" : 2048 x 2732 px
- iPad Pro 11" : 1668 x 2388 px

**Recommandation :** Utilisez l'iPhone 14 Pro ou 15 Pro pour les screenshots principaux.

### Format et Qualité

- **Format :** PNG (transparence) ou JPG (taille réduite)
- **Qualité JPG :** 85-90%
- **Taille max :** 500 KB par image pour le web
- **Optimisation :** Utiliser ImageOptim ou TinyPNG

---

## 🎯 Workflow de Capture

### 1. Préparation

```bash
# Créer les sous-dossiers dans images/
cd /Users/yoyofr/Documents/Dev/modizer/Resources/UserGuide/images/
mkdir -p player browser playlist online search downloads settings visualizer more features
```

### 2. Capture sur Simulateur iOS

Dans Xcode Simulator :
- **Screenshot :** `Cmd + S` (sauvegardé sur le Bureau)
- **Enregistrement :** Utiliser QuickTime Player pour les vidéos

### 3. Capture sur Appareil Physique

Sur iPhone/iPad :
- **Screenshot :** `Volume Up + Side Button` (iPhone X+)
- **Screenshot :** `Home + Side Button` (iPhone 8 et antérieurs)
- **Transférer :** AirDrop ou iCloud Photos

### 4. Organisation des Fichiers

```bash
# Déplacer les screenshots capturés
mv ~/Desktop/Simulator\ Screen\ Shot*.png images/player/
mv ~/Desktop/IMG_*.PNG images/player/

# Renommer selon la convention
cd images/player/
mv "Simulator Screen Shot - iPhone 14 Pro - 2026-01-07 at 19.35.png" player-main-view-portrait.png
```

### 5. Optimisation

```bash
# Avec ImageOptim (Mac)
open -a ImageOptim images/

# Avec TinyPNG CLI (Node.js)
npm install -g tinypng-cli
tinypng images/*.png -k YOUR_API_KEY

# Avec optipng (ligne de commande)
brew install optipng
find images/ -name "*.png" -exec optipng -o5 {} \;
```

---

## 🖼️ Styles de Présentation

### Screenshots Individuels
Utilisez pour montrer un écran complet ou une fonctionnalité spécifique.

### Séquence (2-3 images)
Montrez un workflow en 2-3 étapes (ex: sélection fichier → lecture → visualiseur).

### Comparaison Côte à Côte
Comparez différents modes (ex: mode clair vs mode sombre).

### Détail avec Zoom
Mettez en évidence une zone spécifique avec un zoom ou un encadré.

### Annotations
Ajoutez des flèches, numéros ou texte pour expliquer les éléments.

---

## 🎨 Conseils pour de Belles Captures

### Contenu

✅ **À FAIRE :**
- Utilisez des fichiers musicaux avec de vrais noms (pas "test.mod")
- Montrez des playlists avec plusieurs entrées
- Capturez les visualiseurs en action (effets actifs)
- Incluez du contenu représentatif (vraies pochettes d'albums)
- Variez les formats de fichiers montrés

❌ **À ÉVITER :**
- Fichiers nommés "test", "sample", etc.
- Listes vides
- Lorem ipsum ou texte factice
- Données personnelles sensibles
- Erreurs ou bugs visibles

### Esthétique

✅ **À FAIRE :**
- Mode sombre ET mode clair pour les écrans principaux
- Volume à un niveau normal (50-70%)
- Batterie suffisante (> 50%)
- Heure lisible (10:00, 14:30, etc.)
- Signal réseau présent

❌ **À ÉVITER :**
- Batterie faible (< 20%)
- Mode "Ne pas déranger" actif (sauf si pertinent)
- Notifications non liées à l'app
- Barre de statut incohérente

### Timing

✅ **À FAIRE :**
- Capturez pendant la lecture pour voir les animations
- Montrez les boutons en état actif quand pertinent
- Incluez les indicateurs de progression

❌ **À ÉVITER :**
- États transitoires ou animations à moitié rendues
- Textes tronqués ou scrolling incomplet

---

## 🔧 Outils Recommandés

### Capture
- **Xcode Simulator** (gratuit) - captures parfaites
- **Shottr** (gratuit, Mac) - annotations rapides
- **CleanShot X** (payant, Mac) - captures professionnelles
- **Screenshot Studio** (Mac/Web) - mockups avec device frame

### Édition
- **Sketch** ou **Figma** - composition et annotations
- **Pixelmator Pro** (Mac) - édition rapide
- **Photoshop** - édition avancée

### Optimisation
- **ImageOptim** (gratuit, Mac) - compression sans perte
- **TinyPNG** (web/CLI) - compression PNG/JPG
- **Squoosh** (web, Google) - compression moderne

### Annotations
- **Skitch** (gratuit) - annotations simples
- **Annotate** (Mac) - flèches et texte
- **Markup** (iOS intégré) - édition basique

---

## 📊 Checklist par Section

### Player Screen (12 captures minimum)
- [ ] Vue principale portrait
- [ ] Vue principale landscape
- [ ] Contrôles de lecture
- [ ] Information piste
- [ ] Modes de boucle
- [ ] Modes shuffle
- [ ] Système de notation
- [ ] Sélecteur subsong
- [ ] Sélecteur archive
- [ ] Panneau info
- [ ] Bouton visualiseur
- [ ] Mini player

### Visualizers (10 captures)
- [ ] ProjectM plein écran (2-3 presets)
- [ ] Spectre 3D
- [ ] Paysage 3D
- [ ] Piano 3D
- [ ] Spectre 2D
- [ ] Piano roll
- [ ] Motif MOD
- [ ] Oscilloscope
- [ ] Sélecteur de preset

### File Browser (10 captures)
- [ ] Vue principale
- [ ] Navigation dossiers
- [ ] Barre de recherche
- [ ] Liste fichiers
- [ ] Menu contextuel
- [ ] Détails fichier
- [ ] Créer dossier
- [ ] Renommer
- [ ] Contenu archive
- [ ] Dossier iCloud

---

## 🚀 Script d'Aide au Renommage

Créez ce script pour renommer rapidement vos screenshots :

```bash
#!/bin/bash
# rename-screenshots.sh

echo "Modizer Screenshot Renamer"
echo "=========================="
echo ""
echo "Copiez vos screenshots dans images/temp/"
echo "Ce script vous aidera à les renommer selon la convention."
echo ""

if [ ! -d "images/temp" ]; then
    mkdir -p images/temp
fi

cd images/temp

count=1
for file in *.png *.PNG *.jpg *.JPG; do
    if [ -f "$file" ]; then
        echo ""
        echo "[$count] Fichier: $file"
        echo "Section (player/browser/playlist/online/search/downloads/settings/more):"
        read section
        echo "Description (ex: main-view, playback-controls):"
        read desc
        echo "Variation optionnelle (portrait/landscape/dark/light) [Entrée si aucune]:"
        read var

        if [ -z "$var" ]; then
            newname="${section}-${desc}.png"
        else
            newname="${section}-${desc}-${var}.png"
        fi

        mkdir -p "../${section}"
        mv "$file" "../${section}/${newname}"
        echo "✓ Renommé: ${section}/${newname}"

        ((count++))
    fi
done

echo ""
echo "✓ Terminé! Vérifiez le dossier images/"
```

Utilisation :
```bash
chmod +x rename-screenshots.sh
./rename-screenshots.sh
```

---

## 📝 Notes Importantes

1. **Cohérence :** Utilisez le même appareil/simulateur pour toutes les captures d'une même série

2. **Version iOS :** Notez la version d'iOS utilisée (ex: iOS 17.2)

3. **Localisation :** Capturez en anglais ET en français si possible

4. **Dark Mode :** Priorité aux captures en dark mode, mais incluez aussi light mode

5. **Mise à jour :** Recapturez les écrans modifiés lors des mises à jour de l'app

6. **Backup :** Gardez les originaux non optimisés en HD dans un dossier séparé

---

## ✅ Validation Finale

Avant de publier le guide, vérifiez :

- [ ] Toutes les captures sont nommées correctement
- [ ] Images optimisées (< 500 KB chacune)
- [ ] Format cohérent (toutes PNG ou toutes JPG par section)
- [ ] Résolution appropriée (pas trop petites, pas énormes)
- [ ] Aucune donnée personnelle visible
- [ ] Qualité visuelle acceptable (pas floues, pas pixelisées)
- [ ] Screenshots en contexte (vraies données, pas de contenu factice)
- [ ] Dark mode et light mode représentés
- [ ] Toutes les fonctionnalités principales documentées

---

Pour toute question sur les screenshots, référez-vous à ce guide ou consultez les exemples dans le dossier `images/examples/` (à créer).
