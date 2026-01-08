# Guide d'Intégration iOS

Ce document explique comment intégrer le guide utilisateur HTML dans votre application Modizer.

## 🎯 Méthode 1 : UIKit avec WKWebView

### UserGuideViewController.swift

```swift
import UIKit
import WebKit

class UserGuideViewController: UIViewController {
    private var webView: WKWebView!
    private var activityIndicator: UIActivityIndicatorView!

    override func viewDidLoad() {
        super.viewDidLoad()

        title = "User Guide"
        view.backgroundColor = .systemBackground

        setupWebView()
        setupActivityIndicator()
        loadUserGuide()
        setupNavigationBar()
    }

    private func setupWebView() {
        let configuration = WKWebViewConfiguration()
        configuration.allowsInlineMediaPlayback = true

        // Optionnel : Injecter du JavaScript pour communiquer avec l'app
        let userScript = WKUserScript(
            source: """
            window.webkit.messageHandlers.navigationHandler.postMessage({
                action: 'ready'
            });
            """,
            injectionTime: .atDocumentEnd,
            forMainFrameOnly: true
        )
        configuration.userContentController.addUserScript(userScript)
        configuration.userContentController.add(self, name: "navigationHandler")

        webView = WKWebView(frame: view.bounds, configuration: configuration)
        webView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        webView.navigationDelegate = self
        webView.scrollView.contentInsetAdjustmentBehavior = .never
        view.addSubview(webView)
    }

    private func setupActivityIndicator() {
        activityIndicator = UIActivityIndicatorView(style: .large)
        activityIndicator.center = view.center
        activityIndicator.hidesWhenStopped = true
        view.addSubview(activityIndicator)
        activityIndicator.startAnimating()
    }

    private func setupNavigationBar() {
        // Bouton de partage
        let shareButton = UIBarButtonItem(
            barButtonSystemItem: .action,
            target: self,
            action: #selector(shareGuide)
        )

        // Bouton de fermeture si présenté modalement
        let closeButton = UIBarButtonItem(
            barButtonSystemItem: .close,
            target: self,
            action: #selector(closeGuide)
        )

        navigationItem.rightBarButtonItems = [shareButton]
        if presentingViewController != nil {
            navigationItem.leftBarButtonItem = closeButton
        }
    }

    private func loadUserGuide() {
        guard let htmlPath = Bundle.main.path(
            forResource: "index",
            ofType: "html",
            inDirectory: "UserGuide"
        ) else {
            print("Error: User guide HTML file not found")
            showErrorAlert()
            return
        }

        let htmlURL = URL(fileURLWithPath: htmlPath)
        let htmlFolder = htmlURL.deletingLastPathComponent()
        webView.loadFileURL(htmlURL, allowingReadAccessTo: htmlFolder)
    }

    @objc private func shareGuide() {
        // Partager le lien vers le guide en ligne
        let url = URL(string: "https://yourwebsite.com/modizer/userguide")!
        let activityVC = UIActivityViewController(
            activityItems: [url],
            applicationActivities: nil
        )
        activityVC.popoverPresentationController?.barButtonItem = navigationItem.rightBarButtonItem
        present(activityVC, animated: true)
    }

    @objc private func closeGuide() {
        dismiss(animated: true)
    }

    private func showErrorAlert() {
        let alert = UIAlertController(
            title: "Error",
            message: "Unable to load user guide. Please try again later.",
            preferredStyle: .alert
        )
        alert.addAction(UIAlertAction(title: "OK", style: .default))
        present(alert, animated: true)
    }

    // MARK: - Contrôle depuis Swift

    func setLanguage(_ language: String) {
        webView.evaluateJavaScript(
            "window.ModizerGuide.setLanguage('\(language)')"
        ) { result, error in
            if let error = error {
                print("Error setting language: \(error)")
            }
        }
    }

    func navigateToSection(_ section: String) {
        webView.evaluateJavaScript(
            "window.ModizerGuide.navigateToSection('\(section)')"
        ) { result, error in
            if let error = error {
                print("Error navigating to section: \(error)")
            }
        }
    }
}

// MARK: - WKNavigationDelegate

extension UserGuideViewController: WKNavigationDelegate {
    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
        activityIndicator.stopAnimating()

        // Optionnel : Définir la langue selon les préférences de l'app
        if let preferredLanguage = Locale.current.languageCode {
            setLanguage(preferredLanguage)
        }
    }

    func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
        activityIndicator.stopAnimating()
        print("WebView navigation error: \(error)")
        showErrorAlert()
    }

    func webView(
        _ webView: WKWebView,
        decidePolicyFor navigationAction: WKNavigationAction,
        decisionHandler: @escaping (WKNavigationActionPolicy) -> Void
    ) {
        // Gérer les liens externes
        if let url = navigationAction.request.url,
           url.scheme == "http" || url.scheme == "https",
           navigationAction.navigationType == .linkActivated {
            UIApplication.shared.open(url)
            decisionHandler(.cancel)
            return
        }
        decisionHandler(.allow)
    }
}

// MARK: - WKScriptMessageHandler

extension UserGuideViewController: WKScriptMessageHandler {
    func userContentController(
        _ userContentController: WKUserContentController,
        didReceive message: WKScriptMessage
    ) {
        guard message.name == "navigationHandler",
              let body = message.body as? [String: Any],
              let action = body["action"] as? String else {
            return
        }

        switch action {
        case "ready":
            print("User guide loaded and ready")
        case "sectionChanged":
            if let section = body["section"] as? String {
                print("User navigated to section: \(section)")
                // Vous pouvez tracker les analytics ici
            }
        default:
            break
        }
    }
}
```

## 🎯 Méthode 2 : SwiftUI

### UserGuideView.swift

```swift
import SwiftUI
import WebKit

struct UserGuideView: View {
    @Environment(\.dismiss) private var dismiss
    @State private var currentLanguage = Locale.current.languageCode ?? "en"

    var body: some View {
        NavigationView {
            WebView(language: $currentLanguage)
                .navigationTitle("User Guide")
                .navigationBarTitleDisplayMode(.inline)
                .toolbar {
                    ToolbarItem(placement: .navigationBarTrailing) {
                        ShareLink(item: URL(string: "https://yourwebsite.com/modizer/userguide")!) {
                            Image(systemName: "square.and.arrow.up")
                        }
                    }
                    ToolbarItem(placement: .navigationBarLeading) {
                        Button("Close") {
                            dismiss()
                        }
                    }
                }
        }
    }
}

struct WebView: UIViewRepresentable {
    @Binding var language: String

    func makeUIView(context: Context) -> WKWebView {
        let configuration = WKWebViewConfiguration()
        let webView = WKWebView(frame: .zero, configuration: configuration)
        webView.navigationDelegate = context.coordinator

        // Charger le guide
        if let htmlPath = Bundle.main.path(
            forResource: "index",
            ofType: "html",
            inDirectory: "UserGuide"
        ) {
            let htmlURL = URL(fileURLWithPath: htmlPath)
            let htmlFolder = htmlURL.deletingLastPathComponent()
            webView.loadFileURL(htmlURL, allowingReadAccessTo: htmlFolder)
        }

        return webView
    }

    func updateUIView(_ webView: WKWebView, context: Context) {
        // Mettre à jour la langue si elle change
        webView.evaluateJavaScript("window.ModizerGuide.setLanguage('\(language)')") { _, error in
            if let error = error {
                print("Error updating language: \(error)")
            }
        }
    }

    func makeCoordinator() -> Coordinator {
        Coordinator(self)
    }

    class Coordinator: NSObject, WKNavigationDelegate {
        var parent: WebView

        init(_ parent: WebView) {
            self.parent = parent
        }

        func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
            // Guide chargé
            webView.evaluateJavaScript("window.ModizerGuide.setLanguage('\(parent.language)')") { _, _ in }
        }

        func webView(
            _ webView: WKWebView,
            decidePolicyFor navigationAction: WKNavigationAction,
            decisionHandler: @escaping (WKNavigationActionPolicy) -> Void
        ) {
            if let url = navigationAction.request.url,
               url.scheme == "http" || url.scheme == "https",
               navigationAction.navigationType == .linkActivated {
                UIApplication.shared.open(url)
                decisionHandler(.cancel)
                return
            }
            decisionHandler(.allow)
        }
    }
}

// Preview
struct UserGuideView_Previews: PreviewProvider {
    static var previews: some View {
        UserGuideView()
    }
}
```

## 🔗 Intégration dans l'app existante

### Option A : Depuis un bouton dans les settings

```swift
// Dans votre SettingsViewController ou SettingsView
Button("User Guide") {
    let guideVC = UserGuideViewController()
    let navController = UINavigationController(rootViewController: guideVC)
    present(navController, animated: true)
}
```

### Option B : Depuis le menu principal

```swift
// Ajouter un élément de menu
UIAction(title: "Help", image: UIImage(systemName: "questionmark.circle")) { _ in
    let guideVC = UserGuideViewController()
    navigationController?.pushViewController(guideVC, animated: true)
}
```

### Option C : Onboarding au premier lancement

```swift
func showUserGuideIfNeeded() {
    let hasSeenGuide = UserDefaults.standard.bool(forKey: "hasSeenUserGuide")

    if !hasSeenGuide {
        let guideVC = UserGuideViewController()
        guideVC.modalPresentationStyle = .fullScreen
        present(guideVC, animated: true)

        UserDefaults.standard.set(true, forKey: "hasSeenUserGuide")
    }
}
```

## 📱 Intégration avec le système de langue de l'app

```swift
// Synchroniser la langue du guide avec celle de l'app
extension UserGuideViewController {
    func syncLanguageWithApp() {
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(languageDidChange),
            name: NSNotification.Name("AppLanguageDidChange"),
            object: nil
        )
    }

    @objc private func languageDidChange(_ notification: Notification) {
        if let newLanguage = notification.userInfo?["language"] as? String {
            setLanguage(newLanguage)
        }
    }
}
```

## 🎨 Personnalisation du thème selon l'app

Si vous voulez que le guide reprenne les couleurs de votre app :

```swift
// Injecter du CSS personnalisé
let customCSS = """
:root {
    --primary-color: \(yourAppPrimaryColor.toHex());
    --secondary-color: \(yourAppSecondaryColor.toHex());
}
"""

let script = WKUserScript(
    source: """
    var style = document.createElement('style');
    style.innerHTML = '\(customCSS)';
    document.head.appendChild(style);
    """,
    injectionTime: .atDocumentEnd,
    forMainFrameOnly: true
)
configuration.userContentController.addUserScript(script)
```

## 📊 Analytics et tracking

```swift
// Tracker quand l'utilisateur ouvre le guide
Analytics.logEvent("user_guide_opened", parameters: nil)

// Tracker les sections visitées
extension UserGuideViewController: WKScriptMessageHandler {
    func userContentController(
        _ userContentController: WKUserContentController,
        didReceive message: WKScriptMessage
    ) {
        if let section = message.body as? String {
            Analytics.logEvent("user_guide_section_viewed", parameters: [
                "section": section
            ])
        }
    }
}
```

## 🧪 Tests

```swift
import XCTest
@testable import Modizer

class UserGuideTests: XCTestCase {
    func testUserGuideLoads() {
        let vc = UserGuideViewController()
        vc.loadViewIfNeeded()

        XCTAssertNotNil(vc.webView)
    }

    func testHTMLFileExists() {
        let htmlPath = Bundle.main.path(
            forResource: "index",
            ofType: "html",
            inDirectory: "UserGuide"
        )
        XCTAssertNotNil(htmlPath, "User guide HTML file should exist")
    }

    func testAllResourcesExist() {
        let cssPath = Bundle.main.path(
            forResource: "style",
            ofType: "css",
            inDirectory: "UserGuide/css"
        )
        XCTAssertNotNil(cssPath, "CSS file should exist")

        let jsPath = Bundle.main.path(
            forResource: "app",
            ofType: "js",
            inDirectory: "UserGuide/js"
        )
        XCTAssertNotNil(jsPath, "JavaScript file should exist")
    }
}
```

## 📝 Notes importantes

1. **Bundle Resources** : Assurez-vous que tous les fichiers (HTML, CSS, JS, images) sont ajoutés au target de votre app dans Xcode.

2. **Permissions** : `loadFileURL(_:allowingReadAccessTo:)` nécessite iOS 9+.

3. **Performance** : Le WKWebView met en cache les ressources. Pour forcer le rechargement pendant le développement :
   ```swift
   webView.reloadFromOrigin()
   ```

4. **Offline** : Le guide fonctionne 100% offline une fois intégré dans l'app.

5. **Taille** : Le guide complet pèse environ 50-100 KB, ce qui est négligeable pour la taille de l'app.
