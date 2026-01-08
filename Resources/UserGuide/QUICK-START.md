# Quick Start - Ajouter vos screenshots au guide

## 📸 Vue d'ensemble

Le guide utilisateur est maintenant prêt avec **toutes les sections détaillées de Modizer** et des emplacements de screenshots pour chaque écran. Il vous suffit maintenant d'ajouter vos propres captures d'écran !

## ✅ Ce qui est déjà fait

- ✅ Structure HTML complète avec 9 sections principales
- ✅ Placeholders pour ~80 screenshots identifiés
- ✅ Navigation détaillée entre sections
- ✅ Support multilingue (FR/EN) - prêt à être complété
- ✅ Design responsive et moderne
- ✅ Dossiers images organisés par section
- ✅ Guide complet de nommage des screenshots (SCREENSHOTS.md)

## 🎯 Prochaines étapes rapides

### 1. Prendre les screenshots

Utilisez le guide `SCREENSHOTS.md` pour voir la liste complète, mais voici les **prioritaires** :

**Écran Principal (Player)** - ~12 screenshots
```
images/player/player-main-view-portrait.png
images/player/player-main-view-landscape.png
images/player/player-playback-controls.png
images/player/mini-player.png
...
```

**Navigateur de fichiers** - ~10 screenshots
```
images/browser/browser-main-view.png
images/browser/browser-folder-navigation.png
images/browser/browser-context-menu.png
...
```

**Visualiseurs** - ~10 screenshots
```
images/visualizer/viz-projectm-fullscreen.png
images/visualizer/viz-3d-spectrum.png
images/visualizer/viz-mod-pattern.png
...
```

### 2. Méthode rapide pour capturer

**Sur Simulateur iOS (Xcode):**
```bash
# 1. Lancez Modizer dans le simulateur
# 2. Cmd + S pour capturer (sauvegardé sur le Bureau)
# 3. Déplacez dans le bon dossier
mv ~/Desktop/Simulator*.png images/player/
# 4. Renommez selon la convention
cd images/player
mv "Simulator Screen Shot*.png" player-main-view-portrait.png
```

**Sur Appareil Physique:**
- Volume Up + Side Button pour capturer
- Transférez via AirDrop
- Placez dans les bons dossiers

### 3. Ajouter les screenshots au HTML

Vous avez **2 options** :

#### Option A : Remplacer le placeholder (Simple)

Trouvez le placeholder dans `index.html` :
```html
<div class="screenshot-placeholder" data-screenshot="player-main-view-portrait.png">
    <div class="placeholder-icon">📸</div>
    <p class="placeholder-text">Player - Portrait View</p>
</div>
```

Remplacez par :
```html
<div class="screenshot">
    <img src="images/player/player-main-view-portrait.png" alt="Player - Portrait View">
</div>
```

#### Option B : Ajouter l'image dans le placeholder (Plus simple)

Modifiez juste le placeholder :
```html
<div class="screenshot-placeholder" data-screenshot="player-main-view-portrait.png">
    <img src="images/player/player-main-view-portrait.png" alt="Player - Portrait View">
</div>
```

Le CSS gérera automatiquement l'affichage !

### 4. Optimiser les images (Recommandé)

```bash
# Avec ImageOptim (Mac - gratuit)
open -a ImageOptim images/

# Ou avec optipng (ligne de commande)
brew install optipng
find images/ -name "*.png" -exec optipng -o5 {} \;

# Objectif : < 500 KB par image pour le web
```

### 5. Tester

```bash
# Serveur local pour tester
cd /Users/yoyofr/Documents/Dev/modizer/Resources/UserGuide
python3 -m http.server 8000

# Ouvrir http://localhost:8000
```

## 📋 Liste de priorité des screenshots

### ⭐⭐⭐ HAUTE PRIORITÉ (Faire en premier)

1. **Player principal** (3 screenshots min)
   - Vue portrait
   - Contrôles de lecture
   - Mini player

2. **Browser** (3 screenshots min)
   - Vue principale
   - Navigation dossiers
   - Menu contextuel

3. **Visualiseurs** (3 screenshots min)
   - ProjectM fullscreen
   - Spectre 3D
   - MOD Pattern

### ⭐⭐ PRIORITÉ MOYENNE

4. **Playlists** (2 screenshots)
5. **Online Collections** (2 screenshots)
6. **Settings** (2 screenshots)

### ⭐ PRIORITÉ BASSE (Compléter plus tard)

7. Tous les autres écrans détaillés

## 🎨 Conseils pour de beaux screenshots

### Contenu à montrer

✅ **BON :**
- Fichiers musicaux réels (pas "test.mod")
- Playlists avec plusieurs entrées
- Visualiseurs en action
- Vraies pochettes d'albums
- Variété de formats

❌ **ÉVITER :**
- Fichiers "test" ou "sample"
- Listes vides
- Données personnelles sensibles
- Erreurs visibles
- Batterie < 20%

### Esthétique

- **Mode sombre** pour les screenshots principaux (plus moderne)
- Ajoutez aussi quelques screenshots en **mode clair**
- Volume entre 50-70%
- Heure lisible (10:00, 14:30, etc.)
- Signal réseau présent

## 🚀 Workflow recommandé (30 minutes)

```bash
# 1. Préparer l'app (5 min)
# - Ajoutez quelques vrais fichiers musicaux
# - Créez 1-2 playlists
# - Téléchargez quelques fichiers depuis MODLAND

# 2. Capturer les screenshots prioritaires (15 min)
# - 3 du player
# - 3 du browser
# - 3 des visualiseurs
# - 2 des playlists
# = 11 screenshots essentiels

# 3. Organiser et optimiser (5 min)
cd ~/Desktop
# Renommer selon SCREENSHOTS.md
mv "Simulator Screen Shot - iPhone 14 Pro - 2026-01-07 at 19.35.png" player-main-view-portrait.png
# Déplacer dans les bons dossiers
mv player-*.png images/player/
mv browser-*.png images/browser/
# Optimiser
optipng -o5 images/*/*.png

# 4. Intégrer dans le HTML (5 min)
# Ouvrir index.html et remplacer les placeholders

# 5. Tester (5 min)
python3 -m http.server 8000
# Vérifier que tout s'affiche bien
```

## 🔄 Mise à jour progressive

Pas besoin de tout faire d'un coup ! Procédez ainsi :

### Semaine 1 : MVP (Minimum Viable Product)
- Screenshots prioritaires (11 captures)
- Sections Player, Browser, Visualiseurs complètes
- Guide utilisable et publiable

### Semaine 2 : Complétion
- Ajout des screenshots Playlists, Online, Settings
- ~20 screenshots supplémentaires

### Semaine 3 : Perfectionnement
- Screenshots de tous les détails
- Mode clair ET mode sombre
- Détails des plugins

## 📝 Aide-mémoire : Structure des fichiers

```
UserGuide/
├── index.html                    ← Guide principal (éditer pour ajouter images)
├── SCREENSHOTS.md                ← Liste complète des screenshots
├── QUICK-START.md                ← Ce fichier !
├── README.md                     ← Documentation générale
├── INTEGRATION.md                ← Intégration iOS (Swift/UIKit/SwiftUI)
│
├── images/                       ← Placez vos screenshots ici
│   ├── player/                   ← Screenshots de l'écran principal
│   ├── browser/                  ← Screenshots du navigateur
│   ├── playlist/                 ← Screenshots des playlists
│   ├── online/                   ← Screenshots des collections en ligne
│   ├── search/                   ← Screenshots de la recherche
│   ├── downloads/                ← Screenshots des téléchargements
│   ├── settings/                 ← Screenshots des paramètres
│   ├── visualizer/               ← Screenshots des visualiseurs
│   ├── more/                     ← Screenshots du menu More
│   └── features/                 ← Screenshots des fonctionnalités
│
├── css/
│   └── style.css                 ← Styles (ne pas toucher)
│
└── js/
    ├── i18n.js                   ← Traductions (à compléter)
    └── app.js                    ← Logic (ne pas toucher)
```

## 🌍 Ajouter les traductions françaises

Le guide affiche actuellement le texte en anglais. Pour ajouter le français :

Éditez `js/i18n.js` et complétez la section `fr:` avec les traductions.

**Exemple :**
```javascript
fr: {
    sections: {
        player: {
            title: "Écran Lecteur",
            overview: {
                title: "Vue d'ensemble",
                text: "L'écran lecteur est le cœur de Modizer..."
            },
            // ... etc
        }
    }
}
```

Toutes les clés `data-i18n="..."` dans le HTML correspondent aux traductions.

## ❓ Questions fréquentes

**Q : Les placeholders s'affichent-ils dans l'app iOS ?**
R : Non, seulement les vraies images. Les placeholders sont juste pour le développement.

**Q : Quelle résolution pour les screenshots ?**
R : Utilisez iPhone 14 Pro ou 15 Pro (1179 x 2556 px). Le CSS les redimensionnera automatiquement.

**Q : Dois-je tout capturer en portrait ?**
R : Non ! Ajoutez aussi des screenshots en landscape pour le player et les visualiseurs.

**Q : Puis-je utiliser des screenshots de l'iPad ?**
R : Oui, mais restez cohérent. iPhone recommandé pour la plupart des captures.

**Q : Comment voir mes changements ?**
R : Ouvrez `index.html` dans un navigateur, ou lancez un serveur local : `python3 -m http.server 8000`

## 📞 Besoin d'aide ?

- Consultez `SCREENSHOTS.md` pour la liste complète
- Consultez `README.md` pour la documentation générale
- Consultez `INTEGRATION.md` pour l'intégration iOS

## 🎉 Vous êtes prêt !

Votre guide utilisateur est structuré et prêt à recevoir vos screenshots. Commencez par les 11 screenshots prioritaires et le guide sera déjà excellent !

Bon courage ! 🚀
