# App Intents pour Modizer

Ce document décrit les App Intents disponibles dans Modizer pour Shortcuts et Siri.

## Configuration requise

- iOS 16.0 ou supérieur
- Les App Intents sont automatiquement disponibles après l'installation de l'app

## Intents disponibles

### 1. Contrôle de lecture

#### Play
- **Description** : Démarre la lecture de musique
- **Phrases Siri** :
  - "Play in Modizer"
  - "Start playing Modizer"
  - "Resume Modizer"
- **Usage Shortcuts** : Ajouter l'action "Play" depuis Modizer

#### Pause
- **Description** : Met en pause la lecture
- **Phrases Siri** :
  - "Pause Modizer"
  - "Pause music in Modizer"
- **Usage Shortcuts** : Ajouter l'action "Pause" depuis Modizer

#### Stop
- **Description** : Arrête la lecture
- **Usage Shortcuts** : Ajouter l'action "Stop" depuis Modizer

### 2. Navigation

#### Next Song
- **Description** : Passe à la chanson suivante
- **Phrases Siri** :
  - "Next song in Modizer"
  - "Skip to next in Modizer"
  - "Play next in Modizer"
- **Usage Shortcuts** : Ajouter l'action "Next Song" depuis Modizer

#### Previous Song
- **Description** : Retourne à la chanson précédente
- **Phrases Siri** :
  - "Previous song in Modizer"
  - "Go back in Modizer"
  - "Play previous in Modizer"
- **Usage Shortcuts** : Ajouter l'action "Previous Song" depuis Modizer

#### Play Subsong
- **Description** : Joue un subsong spécifique (pour les fichiers multi-songs)
- **Paramètres** :
  - `subsongIndex` (Int) : Index du subsong à jouer (par défaut: 0)
- **Usage Shortcuts** :
  - Ajouter l'action "Play Subsong"
  - Configurer l'index du subsong désiré

### 3. Seek (Avancer/Reculer)

#### Seek
- **Description** : Se déplace à une position spécifique dans le morceau
- **Paramètres** :
  - `position` (Int) : Position en secondes
  - `relative` (Bool) : Si true, la position est relative à la position actuelle (par défaut: false)
- **Usage Shortcuts** :
  - **Seek absolu** : Définir `position` = 30, `relative` = false → Va à 30 secondes
  - **Seek relatif (avancer)** : Définir `position` = 10, `relative` = true → Avance de 10 secondes
  - **Seek relatif (reculer)** : Définir `position` = -10, `relative` = true → Recule de 10 secondes

### 4. Modes de lecture

#### Toggle Repeat
- **Description** : Active/désactive le mode répétition
- **Paramètres** :
  - `mode` (Int) : 0 = off, 1 = répétition infinie (par défaut: 1)
- **Usage Shortcuts** : Ajouter l'action "Toggle Repeat" et configurer le mode

#### Toggle Shuffle
- **Description** : Active/désactive le mode aléatoire
- **Usage Shortcuts** : Ajouter l'action "Toggle Shuffle"

### 5. Informations

#### Get Now Playing
- **Description** : Obtient les informations sur la chanson en cours de lecture
- **Phrases Siri** :
  - "What's playing in Modizer"
  - "Current song in Modizer"
  - "Now playing in Modizer"
- **Retour** : Une chaîne de caractères avec :
  - État (Playing/Paused)
  - Titre de la chanson
  - Temps écoulé / Temps total
- **Usage Shortcuts** :
  - Ajouter l'action "Get Now Playing"
  - Utiliser le résultat dans d'autres actions (affichage, notification, etc.)

### 6. Playlists (À implémenter)

#### Play Playlist
- **Description** : Joue une playlist spécifique
- **Paramètres** :
  - `playlistName` (String) : Nom de la playlist
- **Note** : Cette fonctionnalité nécessite une implémentation supplémentaire côté playlist

## Exemples d'utilisation dans Shortcuts

### Exemple 1 : Lecture simple
1. Ouvrir l'app Shortcuts
2. Créer un nouveau raccourci
3. Ajouter l'action "Play" depuis Modizer
4. Nommer le raccourci "Lancer Modizer"
5. Le raccourci peut maintenant être lancé depuis Siri ou l'écran d'accueil

### Exemple 2 : Avancer de 30 secondes
1. Créer un nouveau raccourci
2. Ajouter l'action "Seek" depuis Modizer
3. Configurer :
   - Position : 30
   - Relative : Activé
4. Nommer le raccourci "Avancer 30s"

### Exemple 3 : Afficher la chanson en cours
1. Créer un nouveau raccourci
2. Ajouter l'action "Get Now Playing" depuis Modizer
3. Ajouter l'action "Afficher le résultat"
4. Nommer le raccourci "Que joue Modizer ?"

### Exemple 4 : Automatisation matinale
1. Créer un nouveau raccourci "Musique du matin"
2. Ajouter les actions :
   - "Toggle Shuffle" depuis Modizer (activer le mode aléatoire)
   - "Play" depuis Modizer
3. Créer une automatisation :
   - Déclencheur : Heure (ex: 7h00 tous les jours de la semaine)
   - Action : Lancer le raccourci "Musique du matin"

## Notes techniques

### Architecture

Les App Intents sont implémentés dans `/src/ModizerAppIntents.swift` et utilisent :
- **ModizerPlayerBridge** : Classe singleton qui sert de pont entre Swift et le code Objective-C
- **AppShortcutsProvider** : Fournit les suggestions de raccourcis Siri
- **Bridging Headers** : Exposent les classes Objective-C nécessaires (AppDelegate_Phone, DetailViewControllerIphone, ModizMusicPlayer)

### Ajout de nouveaux intents

Pour ajouter un nouvel intent :

1. Créer une nouvelle struct conformant à `AppIntent` dans `ModizerAppIntents.swift`
2. Implémenter les propriétés statiques requises :
   - `title` : Titre localisé
   - `description` : Description de l'intent
   - `openAppWhenRun` : Si l'app doit s'ouvrir
3. Implémenter la méthode `perform()` avec `@MainActor`
4. Ajouter les méthodes nécessaires dans `ModizerPlayerBridge` si besoin
5. (Optionnel) Ajouter un `AppShortcut` dans `ModizerShortcuts` pour les suggestions Siri

### Debugging

- Les App Intents apparaissent automatiquement dans l'app Shortcuts après l'installation
- Pour tester avec Siri, utiliser les phrases suggérées dans les `AppShortcut`
- Utiliser les logs Xcode pour déboguer les appels d'intents

## Limitations actuelles

1. **Playlists** : L'intent "Play Playlist" nécessite une implémentation supplémentaire pour accéder au contrôleur de playlists
2. **Shuffle** : Le toggle shuffle est implémenté mais peut nécessiter des ajustements selon le comportement désiré
3. **Seek** : La conversion position/temps est en millisecondes côté player

## Support

Pour des questions ou des problèmes, consulter le code source dans :
- `/src/ModizerAppIntents.swift`
- `/src/carplay/modizer-Bridging-Header.h`
