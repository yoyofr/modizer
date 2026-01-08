# Modizer User Guide

Guide utilisateur multilingue moderne et responsive pour Modizer.

## 📁 Structure

```
UserGuide/
├── index.html          # Page HTML principale
├── css/
│   └── style.css      # Styles modernes et responsive
├── js/
│   ├── i18n.js        # Traductions (FR/EN)
│   └── app.js         # Logique de navigation et interactions
├── images/            # Dossier pour les captures d'écran et images
└── README.md          # Ce fichier
```

## 🌐 Fonctionnalités

- ✅ **Multilingue** : Support Français et Anglais avec détection automatique
- ✅ **Responsive** : S'adapte à tous les écrans (mobile, tablette, desktop)
- ✅ **Navigation fluide** : Navigation par sections avec animations
- ✅ **Mode sombre** : Support automatique du dark mode système
- ✅ **Moderne** : Design minimaliste et épuré
- ✅ **Accessible** : Optimisé pour l'accessibilité
- ✅ **Print-friendly** : Style d'impression optimisé

## 🚀 Utilisation

### En standalone (navigateur web)

Ouvrez simplement `index.html` dans un navigateur web moderne.

### Intégration dans l'application iOS

#### 1. Ajouter les fichiers au projet Xcode

Glissez-déposez le dossier `UserGuide` dans votre projet Xcode et assurez-vous que :
- "Copy items if needed" est coché
- "Create folder references" est sélectionné
- La target Modizer est sélectionnée

#### 2. Afficher le guide dans un WKWebView

```swift
import WebKit

class UserGuideViewController: UIViewController {
    var webView: WKWebView!

    override func viewDidLoad() {
        super.viewDidLoad()

        // Configure WKWebView
        let configuration = WKWebViewConfiguration()
        webView = WKWebView(frame: view.bounds, configuration: configuration)
        webView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        view.addSubview(webView)

        // Load local HTML file
        if let htmlPath = Bundle.main.path(forResource: "index", ofType: "html", inDirectory: "UserGuide") {
            let htmlURL = URL(fileURLWithPath: htmlPath)
            let htmlFolder = htmlURL.deletingLastPathComponent()
            webView.loadFileURL(htmlURL, allowingReadAccessTo: htmlFolder)
        }
    }
}
```

#### 3. Interagir avec le guide depuis Swift (optionnel)

```swift
// Changer la langue
webView.evaluateJavaScript("window.ModizerGuide.setLanguage('fr')") { result, error in
    if let error = error {
        print("Error changing language: \(error)")
    }
}

// Naviguer vers une section
webView.evaluateJavaScript("window.ModizerGuide.navigateToSection('features')") { result, error in
    if let error = error {
        print("Error navigating: \(error)")
    }
}

// Obtenir la section actuelle
webView.evaluateJavaScript("window.ModizerGuide.getCurrentSection()") { result, error in
    if let section = result as? String {
        print("Current section: \(section)")
    }
}
```

## 📝 Personnalisation

### Ajouter du contenu

1. **Modifier les traductions** : Éditez `js/i18n.js`
2. **Ajouter des sections** : Éditez `index.html` et ajoutez vos sections
3. **Personnaliser les styles** : Modifiez `css/style.css`

### Ajouter une nouvelle langue

Dans `js/i18n.js`, ajoutez votre langue :

```javascript
const translations = {
    en: { ... },
    fr: { ... },
    de: { // Nouvelle langue
        tagline: "Der ultimative Chiptune & Modul-Player",
        // ... autres traductions
    }
};
```

Puis ajoutez un bouton dans `index.html` :

```html
<button class="lang-btn" data-lang="de">DE</button>
```

### Modifier les couleurs

Dans `css/style.css`, modifiez les variables CSS :

```css
:root {
    --primary-color: #6366f1;    /* Couleur principale */
    --secondary-color: #ec4899;  /* Couleur secondaire */
    /* ... */
}
```

## 🌍 Déploiement Web

Pour mettre le guide en ligne :

1. **Hébergement statique** : Uploadez tous les fichiers sur votre serveur web
2. **GitHub Pages** : Committez dans une branche `gh-pages`
3. **Netlify/Vercel** : Déployez directement depuis le dossier UserGuide

### Configuration serveur web recommandée

```nginx
# .htaccess (Apache) ou nginx.conf
location /userguide {
    try_files $uri $uri/ /index.html;
}
```

## 📱 Test sur appareil

Pour tester localement avant intégration :

1. Démarrez un serveur HTTP local :
   ```bash
   cd UserGuide
   python3 -m http.server 8000
   ```

2. Ouvrez `http://localhost:8000` dans votre navigateur

3. Pour tester sur iOS Safari :
   - Trouvez votre IP locale : `ifconfig | grep inet`
   - Ouvrez `http://[VOTRE_IP]:8000` sur votre appareil iOS

## 🎨 Ajout d'images et captures d'écran

Placez vos images dans le dossier `images/` et référencez-les dans le HTML :

```html
<img src="images/screenshot-main.png" alt="Main screen" style="max-width: 100%;">
```

Pour les images responsive :

```html
<picture>
    <source media="(max-width: 768px)" srcset="images/mobile-screen.png">
    <source media="(min-width: 769px)" srcset="images/desktop-screen.png">
    <img src="images/default-screen.png" alt="Screenshot">
</picture>
```

## 🔧 Dépannage

### Le guide ne s'affiche pas dans l'app iOS

- Vérifiez que tous les fichiers sont bien inclus dans le bundle
- Assurez-vous que `allowingReadAccessTo` pointe vers le bon dossier
- Vérifiez les logs pour les erreurs de chargement

### Les traductions ne fonctionnent pas

- Vérifiez que `i18n.js` est chargé avant `app.js`
- Ouvrez la console du navigateur pour voir les erreurs JavaScript
- Vérifiez que tous les attributs `data-i18n` sont bien définis

### Le style ne s'affiche pas correctement

- Vérifiez le chemin relatif vers `css/style.css`
- Assurez-vous que le CSS est bien inclus dans le bundle
- Testez dans un navigateur web d'abord

## 📄 Licence

Ce guide utilisateur fait partie du projet Modizer.

## 🤝 Contribution

Pour ajouter du contenu ou améliorer le guide :

1. Modifiez les fichiers nécessaires
2. Testez dans un navigateur et sur iOS
3. Committez vos changements

## 📞 Support

Pour toute question concernant le guide utilisateur, ouvrez une issue sur GitHub.
